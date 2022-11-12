#pragma once

#include<mysql/mysql.h>
#include<string>
#include<list>
#include<mutex>
#include<semaphore.h>
using namespace std;

//数据库连接池类
class MysqlPool
{
private:
    //数据库主机名
    string server_;
    //数据库端口
    unsigned int port_;
    //数据库用户名
    string user_;
    //数据库密码
    string password_;
    //数据库名字
    string dbname_;
    //空闲连接
    int freeConn_;
    //最大连接数
    int maxConn_;
    //正在连接数
    int curConn_;
    //使用双向链表存储连接，添加或删除连接效率高
    list<MYSQL*> pool_;
    //互斥锁
    mutex mysqlMutex_;
    //信号量
    sem_t sem_;
    MysqlPool();
    ~MysqlPool();
public:
    //单例模式
    static MysqlPool* getInstance();
    //初始化数据库连接池
    void init(string server,unsigned int port,string user,string password,string dbname,int maxConn);
    //获取一个连接
    MYSQL* getConnection();
    //释放一个连接
    bool releaConnection(MYSQL* conn);
    //获取空闲连接数
    int getFreeConnection(){ return freeConn_;};
    //回收数据库连接池
    void destroyPool();
};

