#pragma once

#include"user.h"

//群聊成员信息，继承User类
class GroupUser : public User
{
private:
    string role_;
public:
    void setRole(string role){this->role_=role;}
    string getRole(){return role_;}
};