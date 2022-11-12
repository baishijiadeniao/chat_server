#include "offlinemessagemodel.h"
#include "db.h"

void offlineMsgModel::insert(int userid,string msg_){
    char sql[1024]={0};
    sprintf(sql,"insert into offlineMessage values(%d,'%s')",userid,msg_.c_str());
    MySQL mysql;
    mysql.update(sql);
}

void offlineMsgModel::remove(int userid){
    char sql[1024]={0};
    sprintf(sql,"delete from offlineMessage where userid=%d",userid);
    MySQL mysql;
    mysql.update(sql);
}


vector<string> offlineMsgModel::query(int userid){
    char sql[1024]={0};
    sprintf(sql,"select message from offlineMessage where userid=%d",userid);
    vector<string> vec;
    MySQL mysql;
    MYSQL_RES* res=mysql.query(sql);
    if(res != nullptr){
        MYSQL_ROW row;
        while((row=mysql_fetch_row(res)) != nullptr){
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
    }
    return vec;
}
