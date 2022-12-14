#pragma once
#include<mysql/mysql.h>
#include"mysql_pool.h"
#include<string>

// 数据库配置信息
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat_server";
static int maxConn=10;  //数据库最大连接数

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(std::string sql);
    // 查询操作
    MYSQL_RES* query(std::string sql);
    //获取connection_id
    MYSQL* getConnection();
private:
    //数据库连接池
    MysqlPool* mysqlPool_;
};