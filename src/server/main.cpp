
#include "chatserver.h"
#include "chatservice.h"
#include<json.hpp>
#include<signal.h>
#include<iostream>
using  json=nlohmann::json;

void resetHandler(int){
    chatservice::getInstance()->reset();
    exit(0);
}

int main(){
    EventLoop loop;
    signal(SIGINT,resetHandler);
    InetAddress listenaddr("127.0.0.1",6000);
    ChatServer chatserver(&loop,listenaddr,"my_server");
    chatserver.start();
    //epoll_wait以阻塞的方式等待新用户的连接或读写事件的发生等
    loop.loop();
}
/*
{"msgid":1,"id":1,"password":"123456"}
{"msgid":1,"id":2,"password":"123456"}
{"msgid":3,"name":"li si","password":"123456"}
{"msgid":5,"id":1,"from":"zhangasan","to":2,"msg":"hello"}
{"msgid":6,"id":1,"friendid":2}
*/