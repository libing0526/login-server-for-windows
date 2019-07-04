#include "pch.h"
#include "ClientParse.h"
#include "logger.h"

#pragma comment(lib, "../Include/mysql/lib/libmysql.lib")

#define DB_IP		"localhost"
#define DB_USER		"root"
#define DB_PWD		"575619"
#define DB_NAME		"login"
#define TABLE_NAME	"USER"

CClientParse::CClientParse()
{
	InitializeCriticalSection(&m_section);

	m_nType = CS_UNDEFINED;

	m_pSqlCon = mysql_init((MYSQL*)0);
	if (m_pSqlCon != NULL && mysql_real_connect(m_pSqlCon, DB_IP, DB_USER, DB_PWD, DB_NAME, 3306, NULL, 0))
	{
		if (!mysql_select_db(m_pSqlCon, DB_NAME)) 
		{
			INFO_LOGGER << "Select successfully the database!" << END_LOGGER;
			m_pSqlCon->reconnect = 1;
		}
	}
	else 
	{
		std::string strErr = mysql_error(m_pSqlCon);
		WARNNING_LOGGER << "Connect Database Error " << strErr << END_LOGGER;
		m_pSqlCon = NULL;
	}
}


CClientParse::~CClientParse()
{
	DeleteCriticalSection(&m_section);

	if (m_pSqlCon)
	{
		mysql_close(m_pSqlCon);
	}
}

void CClientParse::Run(const std::string& strMsg)
{
	m_nType = CS_UNDEFINED;

	if (strMsg.empty())
		return;

	//json解析
	Document document;
	document.Parse((char*)strMsg.c_str(), strMsg.length());
	if (document.HasParseError())
	{
		ErrorMsg(ERROR_PARSE_JSON);
		//WARNNING_LOGGER << "解析json错误，json串为;" << strMsg << END_LOGGER;
		return;
	}

	if (!document.IsObject())
	{
		ErrorMsg(ERROR_PARSE_JSON);
		//WARNNING_LOGGER << "请求不是标准的json串" << END_LOGGER;
		return;
	}

	//获取业务类型
	if (document.HasMember("type") && document["type"].IsInt())
	{
		m_nType = document["type"].GetInt();
	}
	if (m_nType == CS_UNDEFINED)
	{
		//WARNNING_LOGGER << "业务类型为: " << m_nType << "，不支持" << END_LOGGER;
		return;
	}

	//std::cout << m_nType << " begin  :" << this  << " " << m_strResult << std::endl;

	//std::cout << m_nType << std::endl;

	//处理不同的业务
	switch (m_nType)
	{
	case CS_LOGIN:
		Login(document);
		break;
	case CS_REGISTER:
		Register(document);
		break;
	case CS_RESETPASSWORD:
		ResetPassWord(document);
		break;
	case CS_BEYONDDEADLINE:
		BeyondDeadLine(document);
		break;
	case CS_GETUSERLEVEL:
		GetUserLevel(document);
		break;
	case CS_RELOGINTHREAD:
		CheckRelogin(document);
		break;
	case CS_HEARTTHREAD:
		m_strResult = "10000";
		break;
	default:
		break;
	}

	//std::cout << m_nType << " end  :" << this << " " << m_strResult << std::endl;
}

void CClientParse::GetReturnValue(char *pBuf, int nSize, unsigned long &nLength)
{
	if (pBuf == NULL)
		return;

	memset(pBuf, 0, nSize);

	if (m_strResult.length() <= 0)
		return;

	//长度做保护
	if (m_strResult.length() > nSize - 1)
	{
		strcpy_s(pBuf, nSize -1, m_strResult.c_str());
		nLength = nSize - 1;
	}
	else
	{
		strcpy_s(pBuf, m_strResult.length() + 1, m_strResult.c_str());
		nLength = m_strResult.length() + 1;
	}
	 
}

void CClientParse::ErrorMsg(const int nErrno /* = ERROR_UNKNOWN*/)
{
	GenericStringBuffer<ASCII<> > JsonStringBuff;
	Writer<GenericStringBuffer<ASCII<> >  > JsonWriter(JsonStringBuff);

	JsonWriter.StartObject();
	JsonWriter.Key("return_code");
	JsonWriter.Int(nErrno);
	JsonWriter.EndObject();

	m_strResult = JsonStringBuff.GetString();
}

std::string CClientParse::GetDefaultExp()
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	int y = st.wYear + 1;	 //获取年份年+1
	int m = st.wMonth;		 //获取当前月份
	int d = st.wDay;		 //获得几号
	char buf[64] = { 0 };
	sprintf_s(buf, "%d%02d%02d", y, m, d);
	return buf;
}

void CClientParse::UpdateLoginState(const std::string& strUser)
{
	EnterCriticalSection(&m_section);

	auto it = s_ClientMap.find(strUser);
	if (it != s_ClientMap.end())
	{
		//已经登录了,覆盖value
		it->second->m_bReLogin = true;
		s_ClientMap[strUser] = this;
	}
	else
	{
		//没有登录，保存
		s_ClientMap[strUser] = this;
	}

	LeaveCriticalSection(&m_section);
}

void CClientParse::Login(Document& doc)
{
	if (!doc.HasMember("userName") || !doc["userName"].IsString())
	{
		WARNNING_LOGGER << "用户名信息不正确" << END_LOGGER;
		return;
	}

	if (!doc.HasMember("passWord") || !doc["passWord"].IsString())
	{
		WARNNING_LOGGER << "密码信息不正确" << END_LOGGER;
		return;
	}

	std::string userName = doc["userName"].GetString();
	std::string sql = "SELECT NAME from USER WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
	if (!mysql_fetch_row(res))
	{
		m_strResult = "2001";
		INFO_LOGGER << "用户: " << userName << " 不存在" << END_LOGGER;
		return;
	}

	std::string passWord = doc["passWord"].GetString();
	sql = "SELECT * from USER WHERE NAME=\'" + userName + "\' and PASSWORD=\'" + passWord + "\'";
	bool exist = false;
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT * from USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	else
	{
		MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
		if (mysql_fetch_row(res))
		{
			m_userName = userName;
			m_strResult = "2000";

			UpdateLoginState(m_userName);

			INFO_LOGGER << userName << ":登陆成功: " << END_LOGGER;
		}
		else
		{
			m_strResult = "2002";
			WARNNING_LOGGER << "账号:" << userName << " 密码:" << passWord << " 不正确" << END_LOGGER;
		}
	}
}

void CClientParse::Register(Document& doc)
{
	std::string userName, passWord, qqCount, expDate, level, userType;

	//用户名必须存在
	if (doc.HasMember("userName") && doc["userName"].IsString())
	{
		userName = doc["userName"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "用户名不存在" << END_LOGGER;
		return;
	}

	//密码必须存在
	if (doc.HasMember("passWord") && doc["passWord"].IsString())
	{
		passWord = doc["passWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "用户名不存在" << END_LOGGER;
		return;
	}

	if (doc.HasMember("qqCount") && doc["qqCount"].IsString())
	{
		qqCount = doc["qqCount"].GetString();
	}

	if (doc.HasMember("exp") && doc["exp"].IsString())
	{
		expDate = doc["exp"].GetString();
	}
	
	if (doc.HasMember("level") && doc["level"].IsString())
	{
		level = doc["level"].GetString();
	}

	if (doc.HasMember("userType") && doc["userType"].IsString())
	{
		userType = doc["userType"].GetString();
	}

	//先看是否账号已存在
	std::string sql = "SELECT NAME from USER WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "1001";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
	if (mysql_fetch_row(res))
	{
		m_strResult = "1002";
		INFO_LOGGER << "账号已存在，请尝试选择其它账号注册！: " << userName << END_LOGGER;
		return;
	}

	//不存在则注册
	std::string expTime = GetDefaultExp();
	//sql = "INSERT INTO USER (NAME, PASSWORD, QQCOUNT, EXP) VALUES (\'" + userName + "', '" + passWord + +"', '" + qqCount + "', '" + expTime + "')";
	sql = "INSERT INTO USER (NAME, PASSWORD, QQCOUNT, EXP, LEVEL, USERTYPE) VALUES (\'" + \
		userName + "', '" + passWord + "', '" + qqCount + "', '" + expTime + "', '" + level + "', '" + userType + "')";
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "1003";
		return;
	}
	m_strResult = "1000";
	INFO_LOGGER << "账号注册成功: " << userName << END_LOGGER;
}

void CClientParse::ResetPassWord(Document& doc)
{
	std::string userName, oldPassWord, newPassWord;

	//用户名必须存在
	if (doc.HasMember("userName") && doc["userName"].IsString())
	{
		userName = doc["userName"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "用户名不存在" << END_LOGGER;
		return;
	}

	//密码必须存在
	if (doc.HasMember("oldPassWord") && doc["oldPassWord"].IsString())
	{
		oldPassWord = doc["oldPassWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "旧密码不存在" << END_LOGGER;
		return;
	}

	//新密码必须存在
	if (doc.HasMember("newPassWord") && doc["newPassWord"].IsString())
	{
		newPassWord = doc["newPassWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "新密码不存在" << END_LOGGER;
		return;
	}

	std::string sql = "SELECT PASSWORD from USER WHERE NAME=\'" + userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "5001";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "5004";
		INFO_LOGGER << "用户: " << userName << " 不存在" << END_LOGGER;
		return;
	}

	//比较库中的密码和传过来的密码
	std::string strData = row[0];
	if (strData.compare(oldPassWord) != 0)
	{
		INFO_LOGGER << "输入的旧密码不正确:" << userName << END_LOGGER;
		m_strResult = "5002";
		return;
	}

	//更新密码
	sql = "UPDATE USER SET PASSWORD=\'" + newPassWord + "\'" + \
		"WHERE NAME=\'" + userName + "\'";;
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		ERROR_LOGGER << "UPDATE USER SET PASSWORD: " << userName << " Failed" << END_LOGGER;
		m_strResult = "5003";
	}
	m_strResult = "0";
	INFO_LOGGER << "RESET PASSWORD SUCCEED: " << userName << END_LOGGER;
}

void CClientParse::GetUserLevel(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	std::string sql = "SELECT LEVEL from USER WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "用户: " << m_userName << " 不存在" << END_LOGGER;
		return;
	}

	//结果赋值
	m_strResult = row[0];
	INFO_LOGGER << "用户:" << m_userName << " 等级: " << m_strResult << END_LOGGER;
}

void CClientParse::BeyondDeadLine(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	std::string sql = "SELECT EXP from USER WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "3";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "用户: " << m_userName << " 不存在" << END_LOGGER;
		return;
	}

	int expDateVal = atoi(row[0]);
	SYSTEMTIME st;
	GetSystemTime(&st);
	int y = st.wYear;  //获取年份年
	int m = st.wMonth; //获取当前月份
	int d = st.wDay;   //获得几号
	char buf[64] = { 0 };
	sprintf_s(buf, "%d%02d%02d", y, m, d);
	int curDate = atoi(buf);
	int subVal = expDateVal - curDate;
	if (subVal >= 0)
	{
		m_strResult = "0";
	}
	else
	{
		m_strResult = "1";
	}
}

void CClientParse::CheckRelogin(Document& doc)
{
	if (m_bReLogin)
	{
		m_strResult = "1";
		INFO_LOGGER << "检测到用户重复登陆，强制关闭！" << m_userName << END_LOGGER;
	}
	else
	{
		m_strResult = "0";
		//INFO_LOGGER << "重复登录的心跳" << END_LOGGER;
	}
}

CRITICAL_SECTION CClientParse::m_section;

std::map<std::string, CClientParse*> CClientParse::s_ClientMap;

void CClientParse::DeleteSelfLoginState(CClientParse* pClient)
{
	EnterCriticalSection(&m_section);

	auto it = s_ClientMap.begin();
	for (; it != s_ClientMap.end(); it++)
	{
		if (it->second == pClient)
		{
			s_ClientMap.erase(it);
			break;
		}
	}

	LeaveCriticalSection(&m_section);
}
