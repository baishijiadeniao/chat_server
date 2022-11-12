#include "Producer.h"
#include<muduo/base/Logging.h>
#include<iostream>

Producer::Producer(string broker):brokerIp_(broker){
    //创建全局配置
    conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    if(conf_->set("bootstrap.servers",brokerIp_,errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Producer conf create error : "<<errstr_;
        exit(1);
    }

    //创建producer实例
    producer_.reset(RdKafka::Producer::create(conf_.get(),errstr_));
    if(!producer_){
        LOG_ERROR<<"producer create error : "<<errstr_;
        exit(1);
    }
    
}

Producer::~Producer(){
    //如果消息队列里还有消息则阻塞，使消息发送完
    while (producer_->outq_len() > 0)
    {
        LOG_ERROR<<" waitint for: "<<producer_->outq_len();
        producer_->flush(5000);
    }
}


bool Producer::produce(const string& topicName,const string &messages){
    //先在哈希表中查询
    // auto it=topicMap_.find(topicName);
    // if (it == topicMap_.end())
    // {
    //     LOG_ERROR<<"could not find topic ";
    //     return false;
    // }

    //生产消息
    void* payload = const_cast<void*>(static_cast<const void*>(topicName.c_str()));
    RdKafka::ErrorCode err= producer_->produce(
			// 主题
			topicName,
			//任何分区：内置分区器将
			//用于将消息分配给基于主题的在消息键上，或没有设定密钥
			RdKafka::Topic::PARTITION_UA,
			// 创建副本？
			RdKafka::Producer::RK_MSG_COPY,
			// 值
			const_cast<char*>(messages.c_str()), messages.size(),
			// 键
			NULL, 0,
			// 投递时间，默认当前时间
			0,
			// 消息头
			NULL,
			NULL);
    if(err != RdKafka::ERR_NO_ERROR){
        LOG_ERROR<<"topic: "<<topicName<<" produce error: "<<RdKafka::err2str(err);
        if (err == RdKafka::ERR__QUEUE_FULL)
        //最大消息数量达到queue.buffering.max.message
        {
            //队列已经满了，阻塞等待Producer生产消息完成,等待最大10秒
            producer_->poll(1000);
        }
        return false;
    }else{
        producer_->poll(0);
        producer_->flush(5000);
        return true;
    }
    // while (producer_->outq_len() > 0)
    // {
    //     LOG_ERROR<<" waitint for: "<<producer_->outq_len();
    //     producer_->flush(5000);
    // }

}

