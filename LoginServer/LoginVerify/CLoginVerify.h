#pragma once
#include <iostream>
#include "..\json\json.h"
#include <WinSock2.h>
enum USER_TYPE {
    NON,
    STOCK,
    FUTURES
};
enum USER_LEVEL{
    NONE,
    PRIMARY,
    INTERMEDIATE,
    SENIOR
};
class CLoginVerify
{
public:
    CLoginVerify();
    // 初始化env
    int InitEnv(Json::Value& serverInfo);
    // 用户登陆
    virtual int Login(Json::Value& userInfo);
    // 用户注册
    virtual int Register(Json::Value& userInfo);
    // 获取最新版本客户端信息
    virtual const std::string GetLatestVersion();
    // 获取系统时间
    virtual const std::string GetSystemTime();
    // 获取用户截止日期
    virtual const std::string GetDeadLine();
    // 判定用户是否失效
    virtual bool BeyondDeadLine();
    // 获取用户类型
    virtual USER_TYPE GetUserType();
    // 获取用户等级
    virtual USER_LEVEL GetUserLevel();
    // 设置用户截止日期
    virtual int SetDeadLine(const Json::Value& sendDeadLine);
    // 清理env
    void cleanEnv();
    // 心跳保护
    void KeepAlive();
    // Get socket
    SOCKET GetSocket();
    //Set Socket
    void SetSocket(SOCKET sock);
    //Reset password
    virtual int ResetPassWord(Json::Value& newPassWd);
public:
    std::string RecvMes();
    void SendJsonMes(const Json::Value& sendJsonMes);
    void SendStrMes(std::string sendStrMes);
    int _keepAlive;

private:
    void CreateLog();
    std::string _userName;
    SOCKET _clientSocket;
};