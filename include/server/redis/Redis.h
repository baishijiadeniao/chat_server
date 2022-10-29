#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
private:
    //用于发布消息的hiredis同步上下文对象
    redisContext* publish_context_;
    //用于订阅消息的hiredis同步上下文对象
    redisContext* subscribe_context_;
    //回调操作，收到订阅的消息，给service上报
    function<void(int,string)> notify_message_handler;
public:
    Redis();
    ~Redis();
    //连接redis服务器
    bool connect();
    //向redis指定的订阅通道channel发布消息
    bool publish(int channel,string message);
    //向redis指定的订阅通道channel订阅消息
    bool subscribe(int channel);
    //向redis指定的订阅通道channel取消订阅
    bool unsubscribe(int channel);
    //在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    //初始化项业务层上报消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);
};