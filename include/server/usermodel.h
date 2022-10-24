#pragma once
#include "db.h"
#include<user.h>

//UserModel表的数据操作类
class UserModel
{
private:
public:
    //User表的增加方法
    bool insert(User &user);
    //查询特定id的信息
    User query(int id);
    //更新用户状态
    bool updateState(User &user);
    //重置用户状态
    bool resetState();
};
