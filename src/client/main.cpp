#include"chatservice.h"
#include"user.h"
#include"group.h"
#include"public.h"
#include<sys/socket.h>
#include <arpa/inet.h>
#include<iostream>
#include<netinet/in.h>
#include<atomic>
#include<semaphore.h>
#include<unordered_map>
#include<functional>
#include<thread>
#include<ctime>
#include<sys/time.h>
using namespace std;

//全局变量
sem_t rwsem;

//显示当前登录用户的基本信息
void showCurrentUserData();

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

//记录的登录状态,使用原子变量，初始化方式和普通布尔值不同
atomic_bool g_isLoginSuccess{false};

//控制主菜单页面程序
bool isMainMenuRunning=false;

//主聊天页面程序
void mainMenu(int);

//接收线程运行函数，用于接收服务端数据
void readTaskHandler(int);

//登陆的响应逻辑
void doLoginResponse(json& responsejs);
//注册的响应逻辑
void doRegResponse(json& responsejs);

//获取当前系统时间
string getCurrentTime();

void help(int clientfd=0,string command="");
void chat(int,string);
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

//系统支持的客户端命令列表，用于help()输出
unordered_map<string,string> commandMap={
    {"help","显示所有支持的命令，格式help"},
    {"chat","一对一聊天，格式chat:friendid:message"},
    {"addfriend","添加好友，格式addfriend:friendid"},
    {"creategroup","创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式addgroup:groupid"},
    {"groupchat","群聊，格式groupchat:groupid:message"},
    {"loginout","注销，格式loginout"}
};

//注册系统支持的客户端命令，添加命令时只用在这两个map中添加即可，不用修改mainMenu代码，满足代码开闭原则
unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}  
};

int main(int argc,char** argv){

    //启动参数
    if(argc<3){
        //cerr输出error
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    char* ip=argv[1];
    int port=atoi(argv[2]);

    //创建client端socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd)
    {
        cout<<"socket create error"<<endl;
        exit(-1);
    }
    struct sockaddr_in address;
    address.sin_family=AF_INET;
    address.sin_port=htons(port);
    address.sin_addr.s_addr=inet_addr(ip);

    //client 与server进行连接
    if(-1==connect(clientfd,(struct sockaddr*)&address,sizeof(address))){
       cerr<<"connetct server error"<<endl;
       close(clientfd);
       exit(-1); 
    }
    //读写线程通信的信号量
    sem_init(&rwsem,0,0);

    //接收端线程只启动一次，因为子线程在recv中被阻塞，不容易退出，多个连接共用一个接收端线程也不影响
    static int threadnum=0;
    if(threadnum==0){
        //创建接收端线程，用于接收服务端发来的信息，发送端和接收端使并行的
        thread readTask(readTaskHandler,clientfd);
        //分离线程，系统自动回收线程资源
        readTask.detach();
        threadnum++;
    }

    for(;;){
        cout<<"================================================"<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"================================================"<<endl;
        cout<<"input your request: ";
        int choice=0;
        cin>>choice;
        //读取掉缓冲区残留的回车符
        cin.get();

        switch(choice)
        {
        case 1: //登录业务
        {
            int id=-1;
            char pwd[50]={0};
            cout<<"id: ";
            cin>>id;
            cin.get();
            cout<<"password: ";
            //不用cin<< ,因为无法读空格
            cin.getline(pwd,50);
    
            json js;
            js["msgid"]=LOGIN_MSG;
            js["id"]=id;
            js["password"]=pwd;
            string request=js.dump();

            g_isLoginSuccess=false;

            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1){
                cerr<<"send login msg error"<<endl;
            }

            //等待子线程处理完相应的相应消息，比如显示离线信息
            sem_wait(&rwsem);
            
            if(g_isLoginSuccess){
                //登录完成，进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: //注册业务
        {
            char name[50]={0};
            char pwd[50]={0};
            cout<<"name: ";
            cin.getline(name,50);
            cout<<"password: ";
            cin.getline(pwd,50);

            json js;
            js["msgid"]=REG_MSG;
            js["name"]=name;
            js["password"]=pwd;
            string request=js.dump();
            cout<<"registering"<<endl;
            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1){
                cerr<<"send register msg error"<<endl;
                exit(-1);
            }

            //等待子线程处理完相应的相应消息
            sem_wait(&rwsem);
        }
            break;
        case 3: //quit业务
        {
            close(clientfd);
            sem_destroy(&rwsem);
            //正常退出
            exit(0);
        }
        default:
        {
            cerr<<"invalid input!"<<endl;
            break;
        }
        }
    }
}

void showCurrentUserData(){
    cout<<"==================login user===================="<<endl;
    cout<<"current login user => id: "<<g_currentUser.getId()<<" name: "<<g_currentUser.getName()<<endl;
    cout<<"==================friend list==================="<<endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user:g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"==================group list===================="<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group:g_currentUserGroupList){
            for(GroupUser &groupuser:group.getUsers()){
                cout<<groupuser.getId()<<" "<<groupuser.getName()<<" "<<groupuser.getState()<<" "<<groupuser.getRole()<<endl;
            }
        }
    }
    cout<<"================================================"<<endl;
}

void doLoginResponse(json& responsejs){
    if(0 != responsejs["errno"].get<int>()){
        //登录失败
        cerr<< responsejs["errmsg"]<<endl;
        g_isLoginSuccess=false;
    }else{

        //登录成功
        //设置g_currentUser
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
        //记录当前用户的好友列表信息
        if(responsejs.contains("friends")){
            //初始化
            g_currentUserFriendList.clear();
            vector<string> vec=responsejs["friends"];
            for(string &str:vec){
                //反序列化
                json js=json::parse(str);

                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);

                g_currentUserFriendList.push_back(user);
            }
        }
        if(responsejs.contains("groups")){
            //初始化
            g_currentUserGroupList.clear();
            vector<string> vec=responsejs["groups"];
            for(string &str:vec){
                //反序列化
                json groupjs=json::parse(str);
                
                Group group;
                group.setId(groupjs["id"].get<int>());
                group.setName(groupjs["groupname"]);
                group.setDesc(groupjs["groupdesc"]);

                vector<string> vec2=groupjs["users"];
                for(string &userstr:vec2){
                    GroupUser user;
                    //需要反序列化
                    json js=json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]); 
                    user.setRole(js["role"]);    
                    group.getUsers().push_back(user);              
                }

                g_currentUserGroupList.push_back(group);
            }
        }
        
        //显示当前用户的基本信息
        showCurrentUserData();
        
        if(responsejs.contains("offlinemsg")){
            vector<string> vec=responsejs["offlinemsg"];
            for(string &str:vec){
                //反序列化
                json js=json::parse(str);

                //判断是个人聊天信息还是群聊信息,输出格式： time [id] name siad: ***
                if(ONE_CHAT_MSG==js["msgid"].get<int>()){
                    cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"
                    <<js["msg"].get<string>()<<endl;
                }else{
                    cout<<js["time"].get<string>()<<" group message ["<<js["groupid"]<<"]"<<js["name"].get<string>()<<" said:"
                    <<js["msg"].get<string>()<<endl;
                }
            }
        }        
        g_isLoginSuccess=true;

    }
}

void doRegResponse(json& responsejs){
    if(0 != responsejs["errno"].get<int>()){
        //注册失败
        cerr<<"name already exits, register error"<<endl;
    }else{
        //注册成功
        cout<<" register success, userid is "<<responsejs["id"]<<", do not forget it!"<<endl;
    }
}

//接收线程运行的循环
void readTaskHandler(int clientfd){
    for(;;){
        //接收数据
        char buffer[1024];
        int len=recv(clientfd,buffer,1024,0);
        if(len==0 || len==-1){
            close(clientfd);
            exit(-1);
        }
        cout<<"len:"<<len<<endl;
        json js=json::parse(buffer);

        int msgtype=js["msgid"].get<int>();
        //处理好友聊天
        if(msgtype==ONE_CHAT_MSG){
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"
            <<js["msg"].get<string>()<<endl;
            continue;
        }
        //处理群聊
        if (msgtype== GROUP_CHAT_MSG){
            cout<<js["time"].get<string>()<<" group message ["<<js["groupid"]<<"]"<<js["name"].get<string>()<<" said:"
            <<js["msg"].get<string>()<<endl;
            continue;
        }
        //登录成功，处理登录响应
        if(msgtype==LOGIN_MSG_ACK){
            doLoginResponse(js);
            //登录结果处理完成，通知主线程
            sem_post(&rwsem);
            continue;
        }
        //注册成功，处理注册响应
        if(msgtype==REG_MSG_ACK){
            cout<<"doRegResponse"<<endl;
            doRegResponse(js);
            //注册结果处理完成，通知主线程
            sem_post(&rwsem);
            continue;
        }
    }
}

//主菜单页面
void mainMenu(int clientfd){
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning){
        cout<<"enter command: ";
        cin.getline(buffer,1024);
        //用于存放命令和参数
        string commandBuffer(buffer);
        //用于存放命令
        string command;
        int idx=commandBuffer.find(":");
        if(-1==idx){
            //该命令无参数
            command=commandBuffer;
        }else{
            //该命令有参数
            command=commandBuffer.substr(0,idx);
        }
        //在命令注册表中寻找命令对应的事件处理器
        auto it=commandHandlerMap.find(command);
        if(it != commandHandlerMap.end()){
            it->second(clientfd,commandBuffer.substr(idx+1,commandBuffer.size()-idx));
        }else{
            cerr<<"invalid input command"<<endl;
        }
    }
}

//显示所有支持的命令
void help(int,string){
    auto it=commandMap.begin();
    for(;it != commandMap.end();it++){
        cout<<it->first<<" "<<it->second<<endl;
    }
    cout<<endl;
}

//一对一聊天
void chat(int clientfd,string str){
    json js;
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"parameter error"<<endl;
        return;
    }

    //从参数中提取friendid
    int friendid=-1;
    friendid=atoi(str.substr(0,idx).c_str());
    //从参数中提取message
    string message=str.substr(idx+1,str.size()-idx);

    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string sendbuf=js.dump();

    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }
}

//添加好友
void addfriend(int clientfd,string str){
    json js;

    //从参数中提取friendid
    int friendid=-1;
    friendid=atoi(str.c_str());
    cout<<"friendid"<<friendid<<endl;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string sendbuf=js.dump();

    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }
}

void addgroup(int clientfd,string str){
    json js;

    //从参数中提取friendid
    int groupid=-1;
    groupid=atoi(str.c_str());

    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string sendbuf=js.dump();

    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }    
}

//创建群聊
void creategroup(int clientfd,string str){
    json js;
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"parameter error"<<endl;
        return;
    }

    //从参数中提取groupname
    string groupname=str.substr(0,idx).c_str();
    //从参数中提取groupdesc
    string groupdesc=str.substr(idx+1,str.size()-idx);

    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string sendbuf=js.dump();

    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }    
}

//发送群聊信息
void groupchat(int clientfd,string str){
    json js;
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"parameter error"<<endl;
        return;
    }

    //从参数中提取friendid
    int groupid=-1;
    groupid=atoi(str.substr(0,idx).c_str());
    //从参数中提取message
    string message=str.substr(idx+1,str.size()-idx);

    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string sendbuf=js.dump();

    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }
}

//退出登录
void loginout(int clientfd,string str){
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string sendbuf=js.dump();
    //发送消息
    int len=send(clientfd,sendbuf.c_str(),strlen(sendbuf.c_str())+1,0); //这里可以用sendbuf.size()吗
    if(len==-1){
        cerr<<"send chat message fail ->"<<sendbuf<<endl;
    }else{
        //退出主界面
        isMainMenuRunning=false;
    }
}


//获取当前系统时间
string getCurrentTime(){
    time_t t=time(NULL);
    struct tm *ptm=localtime(&t);
    char date[60]={0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",ptm->tm_year+1900,ptm->tm_mon+1,
            ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_min);
    return string(date);
}