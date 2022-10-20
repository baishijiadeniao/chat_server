#include "chatservice.h"
#include "public.h"
#include<muduo/base/Logging.h>

chatservice* chatservice::getInstance(){
    static chatservice cs;
    return &cs;
}

chatservice::chatservice(){
    messageHandlerMap_.insert({LOGIN_MSG,std::bind(&chatservice::login,this,_1,_2,_3)});
    messageHandlerMap_.insert({REG__MSG,std::bind(&chatservice::reg,this,_1,_2,_3)});
}

chatservice::~chatservice(){

}

void chatservice::login(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    LOG_INFO<<"do login handle";
}

void chatservice::reg(const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
    LOG_INFO<<"do register handle";
}

MessageHandle chatservice::getHandler(int mesgid){
    auto it=messageHandlerMap_.find(mesgid);
    if(it==messageHandlerMap_.end()){
        //用lambda表达式返回一个空处理器
        return [=](const TcpConnectionPtr& conn,json &js,Timestamp timestamp){
            LOG_ERROR<<"mesgid: "<<mesgid<<" can not find handler";
        };
    }else{
        return  messageHandlerMap_[mesgid];
    }
}