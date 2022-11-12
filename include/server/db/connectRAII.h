#pragma once

#include"mysql_pool.h"

//RAII思想，将资源与对象的生命周期绑定
class connectRAII
{
private:
    MysqlPool* pollRAII_;
    MYSQL* connRAII_;
public:
    connectRAII(MYSQL** conn,MysqlPool* pool);
    ~connectRAII();
};