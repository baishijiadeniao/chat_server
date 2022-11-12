#include "Consumer.h"
#include<thread>
#include<muduo/base/Logging.h>
#include<iostream>

Consumer::Consumer(string broker,string topic):brokerIp_(broker),topicName_(topic){

}

Consumer::~Consumer(){
    //析构对象时取消订阅
    // this->unsubscribe();
    this->consumer_->stop(topic_.get(), partition_);
    RdKafka::wait_destroyed(5000);
}



// bool Consumer::subscribe(string topicName){
//     vector<string> tmp;
//     tmp.push_back(topicName);
//     RdKafka::ErrorCode err=consumer_->subscribe(tmp);
//     if(err != RdKafka::ERR_NO_ERROR){
//         LOG_ERROR<<"topic: "<<topicName<<" subscribe error: "<<RdKafka::err2str(err);
//         return false;
//     }
//     return true;
// }

// void Consumer::unsubscribe(){
//     this->consumer_->unsubscribe();
// }

//在独立线程中接收订阅通道中的消息
void Consumer::consumeWithGuard(){
    while (this->run_)
    {
        unique_ptr<RdKafka::Message> msg(consumer_->consume(topic_.get(), partition_, 10000));
        switch (msg->err())
        {
        case RdKafka::ERR__TIMED_OUT:
            // LOG_ERROR<<" consume error(ERR__TIMED_OUT): "<<msg->errstr();
            break;
        case RdKafka::ERR_NO_ERROR:
            notify_message_handler(atoi(msg->topic_name().c_str()),string(static_cast<char*>(msg->payload())));
            break;
        default:
            break;
        }
        consumer_->poll(0);
    }
}

//初始化项业务层上报消息的回调对象
void Consumer::init_notify_handler(function<void(int,string)> fn){
    this->notify_message_handler=fn;
}

bool Consumer::consume(){
    // 开始消费的偏移地址
	int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;

    // 分区
	partition_ = 0;

    //创建全局配置
    conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    tconf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
    if(conf_->set("bootstrap.servers",brokerIp_,errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    }
    string m_groupID="123456";
    if(conf_->set("group.id", m_groupID, errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    }   
    
    if(conf_->set("enable.partition.eof", "false", errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    }  

    if(conf_->set("auto.offset.reset", "latest", errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    } 

    if(conf_->set("max.partition.fetch.bytes", "1024000", errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    } 

    exampleEventCb_.reset(new ExampleEventCb);
    exampleEventCb_->setConsumer(this);
    if(conf_->set("event_cb", exampleEventCb_.get(), errstr_) !=  RdKafka::Conf::CONF_OK){
        LOG_ERROR<<"Consumer conf create error : "<<errstr_;
        exit(1);
    } 

    //创建consumer实例
    // consumer_.reset(RdKafka::KafkaConsumer::create(conf_.get(),errstr_));
    consumer_.reset(RdKafka::Consumer::create(conf_.get(),errstr_));
    if(!consumer_){
        LOG_ERROR<<"consumer create error : "<<errstr_;
        exit(1);
    }
    //实例化topic
    this->topic_.reset(RdKafka::Topic::create(consumer_.get(),topicName_,tconf_.get(),errstr_));
    if (! topic_.get())
    {
        LOG_ERROR<<"topic create error : "<<errstr_;
        exit(1);
    }

    RdKafka::ErrorCode resp = consumer_->start(topic_.get(), partition_, start_offset);
	if (resp != RdKafka::ERR_NO_ERROR) {
		LOG_ERROR<< "Failed to start consumer: "<< RdKafka::err2str(resp);
		exit(1);
	}

    //订阅topic
    // if(! this->subscribe(topicName_)){
    //     exit(1);
    // }

    //创建线程，负责接收订阅通道的消息，有事件发生则通知给业务端
    //不能使用主线程，因为subscribe订阅通道等待消息的时候会阻塞  
    thread t([&](){
        consumeWithGuard();
    });
    t.detach();

    return true;
}


void ExampleEventCb::event_cb(RdKafka::Event& event){
    switch (event.type()) {
    case RdKafka::Event::EVENT_ERROR:
        if (event.fatal()) {
            LOG_ERROR<< "FATAL ";
            MyConsumer_->setRun(false);
        }
        LOG_ERROR<< "ERROR (" << RdKafka::err2str(event.err())
            << "): " << event.str();
        break;

    case RdKafka::Event::EVENT_STATS:
        LOG_ERROR<< "\"STATS\": " << event.str();
        break;

    case RdKafka::Event::EVENT_LOG:
        // fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(), event.fac().c_str(),
        //     event.str().c_str());
        LOG_INFO<<"LOG-"<<event.severity()<<"-"<<event.fac().c_str()<<": "<<event.str().c_str();
        break;

    default:
        LOG_ERROR<< "EVENT " << event.type() << " ("
            << RdKafka::err2str(event.err()) << "): " << event.str();
        break;
    }
}