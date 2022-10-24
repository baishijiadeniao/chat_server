#pragma once

#include<vector>
#include<string>
using namespace std;

//离线消息表的操作接口类
class offlineMsgModel
{
private:
public:
    //添加用户的离线信息
    void insert(int userid,string msg_);
    //删除用户的离线信息
    void remove(int userid);
    //查询用户的离线信息
    vector<string> query(int userid);
};
