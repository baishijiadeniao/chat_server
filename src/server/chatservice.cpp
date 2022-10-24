#include "chatservice.h"
#include "public.h"
#include "user.h"
#include<muduo/base/Logging.h>

chatservice* chatservice::getInstance(){
    static chatservice cs;
    return &cs;
}

chatservice::chatservice(){
    messageHandlerMap_.insert({LOGIN_MSG,std::bind(&chatservice::login,this,_1,_2,_3)});
    messageHandlerMap_.insert({REG_MSG,std::bind(&chatservice::reg,this,_1,_2,_3)});
    messageHandlerMap_.insert({ONE_CHAT_MSG,std::bind(&chatservice::onechat,this,_1,_2,_3)});
    messageHandlerMap_.insert({ADD_FRIEND_MSG,std::bind(&chatservice::addFriend,this,_1,_2,_3)});
}

chatservice::~chatservice(){

}

void chatservice::reset(){
    usermodel_.resetState();
}

//更新用户状态
void chatservice::login(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    LOG_INFO<<"do login handle";
    int id =js["id"].get<int>();
    string password=js["password"];
    User user=usermodel_.query(id);
    if(id == user.getId() && user.getPwd()==password){
        if (user.getState()== "online")
        {
            //用户已经登录，请勿重复登录
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=2;
            response["errmsg"]="该账号已经登录";
            conn->send(response.dump());
        }else{
            //登录成功
            json response;
            {
                //STL容器是非线程安全的，所以要设置互斥锁
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert({user.getId(),conn});
            }
            //更新状态
            user.setState("online");
            usermodel_.updateState(user);
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["password"]=user.getPwd();

            //查询用户是否有离线消息
            vector<string> vec;
            vec=offlineMsgModel_.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;
                //读取该用户的离线消息后需要把该离线消息从数据库删除
                offlineMsgModel_.remove(id);
            }

            //查询好友列表
            vector<User> userVec;
            userVec=friendmodel_.query(id);
            if(!userVec.empty()){
                vector<string> vec2;
                for(User &user:userVec){
                    json j2;
                    j2["id"]=user.getId();
                    j2["name"]=user.getName();
                    j2["state"]=user.getState();
                    vec2.push_back(j2.dump());
                }
                response["friends"]=vec2;
            }
            
            conn->send(response.dump());
        }
    }else{
        //密码错误或账号不存在
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=2;
        response["errmsg"]="密码错误或账号不存在";
        conn->send(response.dump());
    }

}

//处理注册业务
void chatservice::reg(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    LOG_INFO<<"do register handle";
    string name =js["name"];
    string password=js["password"];
    User user;
    user.setName(name);
    user.setPwd(password);

    bool state=usermodel_.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }else{
        //注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]=1;;
        response["id"]=user.getId();
        conn->send(response.dump());       
    }

}

MessageHandle chatservice::getHandler(int msgid){
    auto it=messageHandlerMap_.find(msgid);
    if(it==messageHandlerMap_.end()){
        //用lambda表达式返回一个空处理器
        return [=](const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
            LOG_ERROR<<"msgid: "<<msgid<<" can not find handler";
        };
    }else{
        return  messageHandlerMap_[msgid];
    }
}

void chatservice::clientCloseException(const TcpConnectionPtr& conn){
    User user;
    {
        lock_guard<mutex> lock(connMutex_);
        for(auto it=userConnMap_.begin();it != userConnMap_.end();it++){
            if(it->second == conn){
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }
    //如果用户存在则修改用户在线状态
    if(user.getId() != -1){
        user.setState("offline");
        usermodel_.updateState(user);
    }
}

//一对一聊天业务
void chatservice::onechat(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    int toid=js["to"].get<int>();
    //使用互斥锁保护，防止用户转发信息时退出登录
    {
        lock_guard<mutex> lock(connMutex_);
        auto it=userConnMap_.find(toid);
        if(it != userConnMap_.end()){
            //用户在线,直接转发消息
            it->second->send(js.dump());
            return;
        }
    }
    //用户不在线，需要先缓存消息，等待用户上线再转发
    offlineMsgModel_.insert(toid,js.dump());
    return;
}


//添加好友信息
void chatservice::addFriend(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    friendmodel_.insert(userid,friendid);
}