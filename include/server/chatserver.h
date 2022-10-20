#pragma once

#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>
#include<muduo/net/InetAddress.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //构造函数设置回调
    ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg);
    void start();
    ~ChatServer();
private:
    //专门处理用户的连接建立和断开事件
    void onConnection(const TcpConnectionPtr& conn);
    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp timestamp);
    EventLoop* loop_;
    TcpServer tcpserver_;
};