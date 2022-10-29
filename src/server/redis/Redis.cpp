#include "Redis.h"
#include <iostream>
// using namespace std;

Redis::Redis():publish_context_(nullptr),subscribe_context_(nullptr){
    
}

Redis::~Redis(){
    if(publish_context_ !=nullptr){
        redisFree(publish_context_);
    }
    if(subscribe_context_ !=nullptr){
        redisFree(subscribe_context_);
    }
}


bool Redis::connect(){
    publish_context_=redisConnect("127.0.0.1",6379);
    if(nullptr==publish_context_){
        cerr<<"connect redis fail"<<endl;
        return false;
    }
    subscribe_context_=redisConnect("127.0.0.1",6379);
    if(nullptr==subscribe_context_){
        cerr<<"connect redis fail"<<endl;
        return false;
    }
    
    //创建线程，负责接收订阅通道的消息，有事件发生则通知给业务端
    //不能使用主线程，因为subscribe订阅通道等待消息的时候会阻塞  
    thread t([&](){
        observer_channel_message();
    });
    t.detach();
    
    cout<< "connect redis-server success"<<endl;

    return true;
}

//向redis指定的订阅通道channel发布消息
bool Redis::publish(int channel,string messages){
    redisReply* reply = (redisReply*)redisCommand(this->publish_context_,
                        "publish %d %s",channel,messages.c_str());
    if(reply==nullptr){
        cerr<<"publish redis fail"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}


//向redis指定的订阅通道channel订阅消息
bool Redis::subscribe(int channel){
    /*subscribe命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不做接收通道，
    否则和notifyMsg线程抢占相应资源，我们专门创建线程运行observer_channel_message函数做接收通道*/
    if(REDIS_ERR==redisAppendCommand(this->subscribe_context_,"subscribe %d",channel)){
        cerr<<"subscribe redis fail"<<endl;
        return false;       
    }

    //循环redisBufferWrite发送缓冲区，直至缓冲区数据被发送完
    int done=0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(this->subscribe_context_,&done)){
            cerr<<"subscribe redis fail"<<endl;
            return false;             
        }
    }

    return true;
}

//向redis指定的订阅通道channel取消订阅
bool Redis::unsubscribe(int channel){
    if(REDIS_ERR==redisAppendCommand(this->subscribe_context_,"unsubscribe %d",channel)){
        cerr<<"unsubscribe redis fail"<<endl;
        return false;       
    }

    //循环redisBufferWrite发送缓冲区，直至缓冲区数据被发送完
    int done=0;
    while(!done){
        if(REDIS_ERR==redisBufferWrite(this->subscribe_context_,&done)){
            cerr<<"unsubscribe redis fail"<<endl;
            return false;             
        }
    }

    return true;    
}

//在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){
    redisReply* reply=nullptr;
    while(REDIS_OK==redisGetReply(this->subscribe_context_,(void**)&reply)){
        //订阅收到的是一个带三元素的数组
        if(reply !=nullptr && reply->element[2] !=nullptr && reply->element[2]->str !=nullptr)
        {
            //给业务层上报通道上发生的消息
            notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<"observer_channel_message quit"<<endl;
}

//初始化项业务层上报消息的回调对象
void Redis::init_notify_handler(function<void(int,string)> fn){
    this->notify_message_handler=fn;
}