#include "chatserver.h"
#include "chatservice.h"
#include<json.hpp>
#include<iostream>
using  json=nlohmann::json;


//构造函数设置回调
ChatServer::ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg):loop_(loop),tcpserver_(loop_,listenAddr,nameArg){
    tcpserver_.setConnectionCallBack(std::bind(&ChatServer::onConnection,this,std::placeholders::_1));
    //_1,_2,_3是占位符，muduo库内部调用时会填充这些参数
    tcpserver_.setMessageCallBack(std::bind(&ChatServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    //4个线程,1个I/O线程，3个工作线程
    tcpserver_.setThreadNum(4);
};

void ChatServer::start(){
    tcpserver_.start();
}

ChatServer::~ChatServer(){};
void ChatServer::onConnection(const TcpConnectionPtr& conn){
    if(!conn->connected()){
        chatservice::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

//专门处理用户的读写事件
void ChatServer::onMessage(const TcpConnectionPtr& conn,
                        Buffer* buffer,
                        timestamp timestamp){
    string buf=buffer->retrieveAllAsString();
    json js=json::parse(buf);
    /*为解耦网络模块和业务模块的代码，不将业务模块的方法写在这，
    而是通过函数回调的方式调用业务模块的方法*/ 
    auto mesgHandler=chatservice::getInstance()->getHandler(js["msgid"].get<int>());
    mesgHandler(conn,js,timestamp);
}