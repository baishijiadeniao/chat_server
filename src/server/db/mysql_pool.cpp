#include"mysql_pool.h"
#include<mymuduo/logger.h>
#include<iostream>

MysqlPool* MysqlPool::getInstance(){
    static MysqlPool mypool;
    return &mypool;
}

MysqlPool::MysqlPool(){
    freeConn_=0;
    curConn_=0;
}

MysqlPool::~MysqlPool(){
    destroyPool();
}

void MysqlPool::init(string server,unsigned int port,string user,string password,string dbname,int maxConn){
    //初始化私有成员
    server_=server;
    port_=port;
    user_=user;
    password_=password;
    dbname_=dbname;
    maxConn_=maxConn;
    //逐个创建连接
    for(int i=0;i<maxConn;i++){
        MYSQL* connect=mysql_init(nullptr);
        if (!connect)
        {
            INFO_LOG("%s","mysql_init error");
            exit(-1);
        }
        connect=mysql_real_connect(connect,server.c_str(),user.c_str(),password.c_str(),
                                dbname.c_str(),port,nullptr,0);
        if (connect != nullptr){
            mysql_query(connect, "set names gbk");
            INFO_LOG("%s","connect mysql success");
        }
        pool_.push_back(connect);                        
    }
    //初始化信号量
    sem_init(&sem_,0,maxConn);
    curConn_=0;
    freeConn_=maxConn;
}

MYSQL* MysqlPool::getConnection(){
    MYSQL* connect=nullptr;
    //如果没有连接了直接退出，值得商榷？
    if (pool_.size()==0)
    {
        return NULL;
    }
    //信号量-1
    sem_wait(&sem_);
    {
        lock_guard<mutex> lock(mysqlMutex_);
        connect=pool_.front();
        pool_.pop_front();
        freeConn_--;
        curConn_++;
    }
    return connect;
}

bool MysqlPool::releaConnection(MYSQL* conn){
    if(conn == NULL){
        return false;
    }
    {
        lock_guard<mutex> lock(mysqlMutex_);
        pool_.push_back(conn);
        freeConn_++;
        curConn_--;
    }
    //信号量+1
    sem_post(&sem_);
    return true;
}

void MysqlPool::destroyPool(){
    {
        lock_guard<mutex> lock(mysqlMutex_);
        //如果有连接被取出就调用destroyConnection，那么取出的连接怎么关闭呢？
        list<MYSQL*>::iterator it;
        for(it=pool_.begin();it != pool_.end();it++){
            MYSQL* connect=*it;
            mysql_close(connect);
        }
        freeConn_=0;
        curConn_=0;
        pool_.clear();
    }
}

