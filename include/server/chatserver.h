#pragma once

#include<mymuduo/EventLoop.h>
#include<mymuduo/TcpServer.h>
#include<mymuduo/InetAddress.h>

using namespace std;
using namespace placeholders;

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
    void onMessage(const TcpConnectionPtr& conn,Buffer* buffer,timestamp timestamp);
    EventLoop* loop_;
    TcpServer tcpserver_;
};