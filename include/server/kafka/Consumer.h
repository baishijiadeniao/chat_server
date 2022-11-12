#pragma once

#include<memory>
#include<unordered_map>
#include<librdkafka/rdkafkacpp.h>
#include<functional>
#include<memory>

using namespace std;

class ExampleEventCb;

//消费者类
class Consumer
{
private:
    //broker服务器ip
    string brokerIp_;

    //topic的名字
    string topicName_;

    //存储错误的字符串
    string errstr_;
    shared_ptr<RdKafka::Topic> topic_;

    int32_t partition_;

    //全局配置
    unique_ptr<RdKafka::Conf> conf_;
    unique_ptr<RdKafka::Conf> tconf_;

    //指向Consumer的指针
    unique_ptr<RdKafka::Consumer> consumer_;

    //回调操作，收到订阅的消息，给service层上报
    function<void(int,string)> notify_message_handler;

    //向指定的topic订阅消息
    bool subscribe(string topicName);

    //向指定的topic取消订阅消息
    void unsubscribe();

    //在独立线程中接收订阅通道中的消息
    void consumeWithGuard();
    //开始消费的标志
    bool run_ = true;
    bool test;
    unique_ptr<ExampleEventCb> exampleEventCb_;
public:
    //完成消费工作的一些初始化操作，然后开始消费
    bool consume();
    
    void setRun(bool run){ run_=run;}

    //初始化向业务层上报消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);

    Consumer(string broker,string topic);
    ~Consumer();
};


class ExampleEventCb : public RdKafka::EventCb{
private:
    Consumer* MyConsumer_;
public:
    void setConsumer(Consumer* con){ MyConsumer_=con;}
	void event_cb(RdKafka::Event& event) override; 
};