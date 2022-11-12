#include"connectRAII.h"

//双重指针作为函数参数，可以在函数函数内部修改外部指针的值。就比如如果你想在函数内部修改外部变量的值就必须要传递该变量的指针
connectRAII::connectRAII(MYSQL** conn,MysqlPool* pool){
    *conn=pool->getConnection();
    connRAII_=*conn;
    pollRAII_=pool;
}

//析构函数自动释放资源
connectRAII::~connectRAII(){
    pollRAII_->releaConnection(connRAII_);
}