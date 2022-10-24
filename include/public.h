#pragma once

//service和client的公共文件
//消息类型枚举
enum EnMsgType{
    //登陆消息
    LOGIN_MSG=1,
    LOGIN_MSG_ACK,
    //注册消息
    REG_MSG,
    //注册响应消息
    REG_MSG_ACK,
    //聊天消息
    ONE_CHAT_MSG,
    //添加好友消息
    ADD_FRIEND_MSG
};