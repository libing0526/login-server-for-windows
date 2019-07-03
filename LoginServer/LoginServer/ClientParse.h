#pragma once
#include <string>
#include <map>

#include "Include/mysql/include/mysql.h"

#include "Include/rapidjson/rapidjson.h"
#include "Include/rapidjson/document.h"
#include "Include/rapidjson/prettywriter.h"  
#include "Include/rapidjson/stringbuffer.h" 
using namespace rapidjson;

#include <windows.h>

//客户端业务类，一个连接对应一个类，此类只处理业务，不负责网络通信
class CClientParse
{
public:
	CClientParse();
	~CClientParse();

public:
	//解析客户端传来的字符串消息
	void Run(const std::string& strMsg);

	//获取业务类的处理结果
	void GetReturnValue(char *pBuf, int nSize, unsigned long &nLength);

private:
	//拼接错误信息用于返回
	void ErrorMsg(const int nErrno = -1);

	//获取默认的使用期限
	std::string GetDefaultExp();

	//校验是否重复登录
	void UpdateLoginState(const std::string& strUser);

private:
	//登录
	void Login(Document& doc);

	//注册
	void Register(Document& doc);

	//重置密码
	void ResetPassWord(Document& doc);

	//获取用户级别
	void GetUserLevel(Document& doc);	

	//获取使用时限
	void BeyondDeadLine(Document& doc);

	//重复登录的校验
	void CheckRelogin(Document& doc);

private:
	std::string m_strResult;

	int m_nType;

	MYSQL *m_pSqlCon = nullptr;	//数据库连接

	std::string m_userName;		//用户名

	static CRITICAL_SECTION m_section;	//锁

public:
	bool m_bReLogin = false;	//是否是重复登录

	static std::map<std::string, CClientParse*> s_ClientMap;	//保存用户名和client信息，用于处理互踢

	static void DeleteSelfLoginState(CClientParse* pClient);	//清除当前的登录状态
};

