#pragma once

#include "db.h"
#include"groupuser.h"
#include<vector>
#include<string>
using namespace std;

//群信息类
class Group
{
private:
    int id_;
    string name_;
    //群描述
    string desc_;
    //将查到的群成员数据放到缓存中，防止多次连接数据库造成性能下降
    vector<GroupUser> users;
public:
    void setId(int id){this->id_=id;}
    void setName(string name){this->name_=name;}
    void setDesc(string desc){this->desc_=desc;}

    int getId(){return id_;}
    string getName(){return name_;}
    string getDesc(){return desc_;}
    vector<GroupUser>& getUsers(){return this->users;}
    Group(int id=-1,string name="",string desc=""):
                            name_(name),
                            id_(id),desc_(desc){};
    ~Group(){};
};