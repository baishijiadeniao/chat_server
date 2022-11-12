#include "db.h"
#include"connectRAII.h"
#include<mymuduo/logger.h>


// 初始化数据库连接
MySQL::MySQL()
{
    mysqlPool_=MysqlPool::getInstance();
    //确保只初始化一次
    static int initNum=0;
    if (initNum==0)
    {
        mysqlPool_->init(server,3306,user,password,dbname,maxConn);
        initNum++;
    }
}

MySQL::~MySQL(){

}

// 更新操作
bool MySQL::update(std::string sql)
{
    MYSQL *_conn=nullptr;
    connectRAII mysqlcon(&_conn,mysqlPool_);
    if (mysql_query(_conn, sql.c_str()))
    {
    INFO_LOG("%s : %d : %s 更新失败!",__FILE__,__LINE__,sql.c_str());
    return false;
    }
    return true;
}

// 查询操作
MYSQL_RES* MySQL::query(std::string sql)
{
    MYSQL *_conn=nullptr;
    connectRAII mysqlcon(&_conn,mysqlPool_);
    if (mysql_query(_conn, sql.c_str()))
    {
    INFO_LOG("%s : %d : %s 查询失败!",__FILE__,__LINE__,sql.c_str());
    return nullptr;
    }
    return mysql_use_result(_conn);
}
