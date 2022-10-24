#pragma once

#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>
#include<muduo/net/InetAddress.h>
#include<unordered_map>
#include<functional>
#include<json.hpp>
#include<mutex>
#include"usermodel.h"
#include"friendmodel.h"
#include"offlinemessagemodel.h"
using namespace muduo;
using namespace muduo::net;
using  json=nlohmann::json;

using MessageHandle=std::function<void(const TcpConnectionPtr& conn,json &js,Timestamp timestamp)>;

//单例模式
//聊天服务器业务类
class chatservice
{
private:
    //存储消息id和其对应的业务处理方法
    std::unordered_map<int,MessageHandle> messageHandlerMap_;
    //存储在线用户的通信连接
    std::unordered_map<int,TcpConnectionPtr>  userConnMap_;
    //互斥锁
    mutex connMutex_;
    //User操作类对象
    UserModel usermodel_;
    friendModel friendmodel_;
    offlineMsgModel offlineMsgModel_;
    //单例模式私有化构造函数
    chatservice();
    ~chatservice();
    chatservice(const chatservice&);
public:
    //单例模式获取静态对象接口
    static chatservice* getInstance();
    //处理登录业务
    void login(const TcpConnectionPtr& conn,json &js,Timestamp timestamp);
    //处理注册业务
    void reg(const TcpConnectionPtr& conn,json &js,Timestamp timestamp);
    //获取消息对应的处理器
    MessageHandle getHandler(int msgid);
    //关闭连接时将conn从userConnMap_移除并修改user状态
    void clientCloseException(const TcpConnectionPtr&);
    //聊天业务
    void onechat(const TcpConnectionPtr& conn,json &js,Timestamp timestamp);
    //服务器断开重置用户状态
    void reset();
    //添加好友
    void addFriend(const TcpConnectionPtr& conn,json &js,Timestamp timestamp);
};