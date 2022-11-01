#include"groupmodel.h"
#include<iostream>

bool groupModel::createGroup(Group &group){
    char sql[1024]={0};
    sprintf(sql,"insert into AllGroup(groupname,groupdesc) values('%s','%s')",
            group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    //groupname不可以重复，否则不能创建成功
    if(mysql.update(sql)){
        //获取插入成功的用户数据生成的主键id;
        MYSQL_RES* res=mysql.query("select max(id) from AllGroup");
        if(res != nullptr){
            MYSQL_ROW row=mysql_fetch_row(res);
            group.setId(atoi(row[0]));
        }
        return true;
    }
    return false;
}

bool groupModel::addGroup(int userid,int groupid,string role){
    char sql[1024]={0};
    sprintf(sql,"insert into GroupUser values(%d,%d,'%s')",groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.update(sql)){
        return true;
    }
    return false;
}

vector<Group> groupModel::queryGroups(int userid){
    /*先查询用户所在的所有群信息， 然后再使用多表联合查询查询用户所在群聊的其他成员信息并放在缓存中*/
    char sql[1024]={0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on b.groupid=a.id where b.userid=%d",userid);
    
    vector<Group> groupvec;
    MySQL mysql;
    MYSQL_RES* res=mysql.query(sql);
    if(res != nullptr){
        MYSQL_ROW row;
        while((row=mysql_fetch_row(res)) != nullptr){
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupvec.push_back(group);
        }
        mysql_free_result(res);
    }
    for(Group &group:groupvec){
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User a inner join GroupUser b on b.userid=a.id where b.groupid=%d",group.getId());
        MySQL mysql;
        MYSQL_RES* res=mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res)) != nullptr){
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);
                group.getUsers().push_back(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return groupvec;
}

//根据指定的groupid查询群组用户id列表，列表不包括自己的id，主要用于发送群聊消息
vector<int> groupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024]={0};
    sprintf(sql,"select userid from GroupUser where groupid = %d and userid != %d",groupid,userid);
    vector<int> idVec;
    MySQL mysql;
    MYSQL_RES* res=mysql.query(sql);
    if(res != nullptr){
        MYSQL_ROW row;
        while((row=mysql_fetch_row(res)) != nullptr){
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}