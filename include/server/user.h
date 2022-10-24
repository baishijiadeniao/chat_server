#pragma once

#include "db.h"
#include<string>
using namespace std;

//User表
class User
{
private:
    int id_;
    string name_;
    string password_;
    string state_;
public:
    void setId(int id){this->id_=id;}
    void setName(string name){this->name_=name;}
    void setPwd(string password){this->password_=password;}
    void setState(string state){this->state_=state;}

    int getId(){return id_;}
    string getName(){return name_;}
    string getPwd(){return password_;}
    string getState(){return state_;};
    User(int id=-1,string name="",string password="",string state="offline"):
                            name_(name),
                            id_(id),password_(password),
                            state_(state){};
    ~User(){};
};