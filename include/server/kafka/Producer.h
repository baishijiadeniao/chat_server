#pragma once

#include<memory>
#include<unordered_map>
#include<librdkafka/rdkafkacpp.h>
#include<librdkafka/rdkafkacpp.h>
#include<functional>

using namespace std;

class Producer
{
private:
    //broker服务器ip
    string brokerIp_;

    //存储错误的字符串
    string errstr_;

    //存储topicName和RdKafka::Topic*的对应关系
    unordered_map<string,shared_ptr<RdKafka::Topic>> topicMap_;

    //全局配置
    unique_ptr<RdKafka::Conf> conf_;
    unique_ptr<RdKafka::Conf> tconf_;

    //指向Producer的指针
    unique_ptr<RdKafka::Producer> producer_;
    
    // RdKafka::DeliveryReportCb* m_dr_cb;
    // RdKafka::EventCb* m_event_cb;
    // RdKafka::PartitionerCb* m_partitioner_cb;


public:
    //向指定的topic发布消息
    bool produce(const string& topicName,const string &messages);

    Producer(string broker);
    ~Producer();
};