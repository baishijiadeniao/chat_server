#pragma once

#include"group.h"

class groupModel
{
private:
public:
    //创建群
    bool createGroup(Group &group);
    //加入群
    bool addGroup(int userid,int groupid,string role);
    //查询某用户已加入的群
    vector<Group> queryGroups(int userid);
    //查询群聊中的其他人，用于发送群聊消息
    vector<int> queryGroupUsers(int userid,int groupid);
    
};
