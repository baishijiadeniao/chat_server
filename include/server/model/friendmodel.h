#pragma once

#include "db.h"
#include"user.h"
#include<vector>

class friendModel
{
private:
    /* data */
public:
    //freidnid表的增加方法
    void insert(int userid,int friendid);
    //查询特定id的好友列表
    vector<User> query(int userid);
};