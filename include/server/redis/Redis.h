#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
private:
    //用于缓存的hiredis同步上下文对象
    redisContext* cache_;
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
    //分布式锁
    //向redis指定的订阅通道channel发布消息
    bool publish(int channel,string message);
    //向redis指定的订阅通道channel订阅消息
    bool subscribe(int channel);
    //向redis指定的订阅通道channel取消订阅
    bool unsubscribe(int channel);
    //在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    //初始化向业务层上报消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);
    //缓存
    //获取key对应的键
    string get(int key);
    //添加键值对
    bool set(int key,string value);
    //删除缓存
    bool del(int key);
    //添加哈希值
    bool hset(string key,string field,int value);
    //获取哈希值
    string hget(string key,string field);
    //设置redis缓存时间
    bool expire(string key,int second);  
};