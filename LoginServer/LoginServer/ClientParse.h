#pragma once
#include <string>
#include <map>

#include "mysql/include/mysql.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"  
#include "rapidjson/stringbuffer.h" 
using namespace rapidjson;

#include "object_pool.h"
#include "logger.h"

#define DB_IP		"47.102.96.37"//"localhost"//
#define DB_USER		"lblogin"
#define DB_PWD		"575619"
#define DB_NAME		"stockalert"
#define TABLE_NAME	"user"


//数据库连接类
typedef struct SqlConNode
{
	MYSQL *pSqlCon;	//数据库连接对象
	SqlConNode* pNext;	//下一个节点

	SqlConNode()
	{
		pSqlCon = NULL;
		pNext = NULL;

		Init();
	}
	~SqlConNode()
	{
		Unit();
	}

	void Init()
	{
		pSqlCon = mysql_init((MYSQL*)0);
		if (pSqlCon)
		{
			char value = 1;
			mysql_options(pSqlCon, MYSQL_OPT_RECONNECT, &value);
			if (mysql_real_connect(pSqlCon, DB_IP, DB_USER, DB_PWD, DB_NAME, 3306, NULL, 0))
			{
				if (!mysql_select_db(pSqlCon, DB_NAME))
				{
					mysql_query(pSqlCon, "SET NAMES UTF8");
					INFO_LOGGER << "Select successfully the database!" << END_LOGGER;
				}
			}
			else
			{
				std::string strErr = mysql_error(pSqlCon);
				WARNNING_LOGGER << "Connect Database Error " << strErr << END_LOGGER;
				pSqlCon = NULL;
			}
		}
		else
		{
			std::string strErr = mysql_error(pSqlCon);
			WARNNING_LOGGER << "Connect Database Error " << strErr << END_LOGGER;
			pSqlCon = NULL;
		}
	}

	void Unit()
	{
		if (pSqlCon)
		{
			mysql_close(pSqlCon);
		}
	}
};

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

	//获取用户名
	inline std::string GetUserName() const {
		return m_userName;
	}

private:
	//拼接错误信息用于返回
	void ErrorMsg(const int nErrno = -1);

	//获取默认的使用期限
	std::string GetDefaultExp();

	//校验是否重复登录
	void UpdateLoginState(const std::string& strUser);

	//清除某个用户的登陆态
	void RemoveLoginState(const std::string& strUser);

private:
	//登录
	void Login(Document& doc);

	//注册
	void Register(Document& doc);

	//重置密码
	void ResetPassWord(Document& doc);

	//获取用户级别
	void GetUserLevel(Document& doc);	

	//获取用户函数控制权限
	void GetFunctionPermission(Document& doc);

	//获取使用时限
	void BeyondDeadLine(Document& doc);

	//重复登录的校验
	void CheckRelogin(Document& doc);

private:
	std::string m_strResult;	//此处简单处理，直接用字符串了，可以自己拼接json串

	std::string m_userName;		//用户名

	std::string m_userId;		//用户Id

	static CLock s_clientLock;	//锁

	static CObjectPool<SqlConNode> s_sqlPool;	//数据库池

public:
	bool m_bReLogin = false;	//是否是重复登录

	static std::map<std::string, CClientParse*> s_ClientMap;	//保存用户名和client信息，用于处理互踢

	static void DeleteSelfLoginState(CClientParse* pClient);	//清除当前的登录状态
};

