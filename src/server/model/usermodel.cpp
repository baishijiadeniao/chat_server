#include "user.h"
#include "usermodel.h"

//User表的增加方法
bool UserModel::insert(User &user){
    char sql[1024]={0};
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
    MySQL mysql;
    if(mysql.update(sql)){
        //获取插入成功的用户数据生成的主键id，考虑到id是自增约束，所以直接查询最大的id
        MYSQL_RES* res=mysql.query("select max(id) from User");
        if(res != nullptr){
            MYSQL_ROW row=mysql_fetch_row(res);
            user.setId(atoi(row[0]));
        }
        mysql_free_result(res);
        return true;
    }
    return false;
}

//查询特定id的信息
User UserModel::query(int id){
    char sql[1024]={0};
    sprintf(sql,"select * from User where id = %d",id);
    MySQL mysql;
    MYSQL_RES* res=mysql.query(sql);
    if(res != nullptr){
        MYSQL_ROW row=mysql_fetch_row(res);
        if(row != nullptr){
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    mysql_free_result(res);
    return User();
}

//更新用户状态
bool UserModel::updateState(User &user){
    char sql[1024]={0};
    sprintf(sql,"update User set state='%s' where id = %d",user.getState().c_str(),user.getId());
    MySQL mysql;
    if(mysql.update(sql)){
        return true;
    }
    return false;
}

bool UserModel::resetState(){
    char sql[1024]="update User set state='offline' where state ='online'";
    MySQL mysql;
    if(mysql.update(sql)){
        return true;
    }
    return false;
}