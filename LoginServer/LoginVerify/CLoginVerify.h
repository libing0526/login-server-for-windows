/*
**@autoer libing
**@describe
	一个客户端只有唯一一个CLoginVerify对象，对应着一个登录角色，所有函数参数都是json格式的字符串，便于拓展。
	至于函数写成virtual，万一你要二次封装呢（—_—）
**@useway
	1，InitEnv初始化socket连接
	2，Login登录，登录成功后可以GetDeadLine获取用户使用期限，GetUserLevel获取用户等级做业务处理
	3，登录成功后，调用KeepAlive后开启重复登录校验线程（同时处理了心跳）
   	  在客户端程序中开启一个定时器线程来校验CLoginVerify的clientState_的心跳校验
    （这一步做的不合理，如果有兴趣你可以改为回调或者接口的机制，聪明的你肯定一点就透）
*/

#pragma once
#include <iostream>
#include <WinSock2.h>

#include "../Include/rapidjson/rapidjson.h"
#include "../Include/rapidjson/document.h"
#include "../Include/rapidjson/prettywriter.h"  
#include "../Include/rapidjson/stringbuffer.h" 
using namespace rapidjson;

//用户类型
enum USER_TYPE {
    NON,
    STOCK,
    FUTURES
};

//用户级别
enum USER_LEVEL{
    NONE,
    PRIMARY,
    INTERMEDIATE,
    SENIOR
};

enum CLIENT_STATE
{
	CLIENT_INITIAL = -1,	//初始状态
	CLIENT_CONNECTED = 0,		//连接中
	CLIENT_RELOGIN = 1,		//重复登录
	CLIENT_DISCONNECT = 2	//断连
};

class CLoginVerify
{
public:
    CLoginVerify();

    // 初始化env
    int InitEnv(const char* pServerInfo);

    // 用户登陆
    virtual int Login(const char* pUserInfo);

    // 用户注册
    virtual int Register(const char* pUserInfo);

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
    virtual int SetDeadLine(const char* pDeadLine);

	//Reset password
	virtual int ResetPassWord(const char* pUserInfo);

    // 清理env
    void cleanEnv();

    // 心跳保护
    void KeepAlive();

    // Get socket
    SOCKET GetSocket();

    //Set Socket
    void SetSocket(SOCKET sock);

private:
    void CreateLog();

private:
    std::string userName_;

    SOCKET clientSocket_;

public:
	int clientState_;		//客户端连接状态
};