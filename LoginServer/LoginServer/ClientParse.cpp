#include "pch.h"
#include "ClientParse.h"


std::string string2UTF8(const std::string & str) {
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴  
	ZeroMemory(pwBuf, nwLen * 2 + 2);

	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	char * pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	std::string retStr(pBuf);

	delete[]pwBuf;
	delete[]pBuf;

	pwBuf = NULL;
	pBuf = NULL;

	return retStr;
}

CClientParse::CClientParse()
{

}

CClientParse::~CClientParse()
{
	
}

void CClientParse::Run(const std::string& strMsg)
{
	int nType = CS_UNDEFINED;

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
		nType = document["type"].GetInt();
	}
	if (nType == CS_UNDEFINED)
	{
		//WARNNING_LOGGER << "业务类型为: " << m_nType << "，不支持" << END_LOGGER;
		return;
	}

	//std::cout << m_nType << " begin  :" << this  << " " << m_strResult << std::endl;

	//std::cout << m_nType << std::endl;

	//处理不同的业务
	switch (nType)
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
	case CS_GETFUNCTIONPERMISSION:
		GetFunctionPermission(document);
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
	CAutoLock lck(&s_clientLock);

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
}

void CClientParse::RemoveLoginState(const std::string& strUser)
{
	CAutoLock lck(&s_clientLock);

	s_ClientMap.erase(strUser);
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

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		WARNNING_LOGGER << "获取空闲节点失败！" << END_LOGGER;
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	std::string userName = doc["userName"].GetString();
	std::string sql = "SELECT NAME from user WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		
		WARNNING_LOGGER << "SELECT NAME from user WHERE NAME=" << userName << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "-3";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	if (!mysql_fetch_row(res))
	{
		m_strResult = "-1";
		INFO_LOGGER << "用户: " << userName << " 不存在" << END_LOGGER;
		s_sqlPool.Recycle(pNode);
		return;
	}

	std::string passWord = doc["passWord"].GetString();
	sql = "SELECT * from user WHERE NAME=\'" + userName + "\' and PASSWORD=\'" + passWord + "\'";
	rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT * from user WHERE NAME= " << userName << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "-3";
		s_sqlPool.Recycle(pNode);
		return;
	}
	else
	{
		MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
		MYSQL_ROW row = NULL;
		if (row = mysql_fetch_row(res))
		{
			if (userName.compare(m_userName) != 0 && !m_userName.empty())
			{
				//清除之前用户的登录状态
				RemoveLoginState(m_userName);
			}
			m_userName = userName;
			//m_strResult = "2000";
			m_strResult = row[0];	//保存用户id
			m_userId = m_strResult;

			UpdateLoginState(m_userName);

			INFO_LOGGER << userName << ":登陆成功: " << END_LOGGER;
		}
		else
		{
			m_strResult = "-2";
			WARNNING_LOGGER << "账号:" << userName << " 密码:" << passWord << " 不正确" << END_LOGGER;
		}
	}

	s_sqlPool.Recycle(pNode);
}

void CClientParse::Register(Document& doc)
{
	std::string userName, passWord, qqCount, expDate, level, userType, referCode;

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

	//经销商推荐码不存在，默认给0
	if (doc.HasMember("referCode") && doc["referCode"].IsString())
	{
		referCode = doc["referCode"].GetString();
		if (referCode.empty())
		{
			referCode = "0";
		}
	}
	else
	{
		referCode = "0";
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

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		WARNNING_LOGGER << "获取空闲节点失败！" << END_LOGGER;
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	//先看是否账号已存在
	std::string sql = "SELECT NAME from user WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME user WHERE NAME= " << userName << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "1001";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	if (mysql_fetch_row(res))
	{
		m_strResult = "1002";
		INFO_LOGGER << "账号已存在，请尝试选择其它账号注册！: " << userName << END_LOGGER;
		s_sqlPool.Recycle(pNode);
		return;
	}

	//再看经销商码是否存在，是否无效
	sql = "SELECT status from saler WHERE refer_code=\'" + referCode + "\'";
	rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << sql << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "1001";
		s_sqlPool.Recycle(pNode);
		return;
	}
	res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "1003";
		s_sqlPool.Recycle(pNode);
		return;
	}
	else
	{
		int nStatus = atoi(row[0]);
		if (nStatus != 0)
		{
			m_strResult = "1004";
			s_sqlPool.Recycle(pNode);
			return;
		}
	}

	//不存在则注册
	std::string expTime = GetDefaultExp();
	//sql = "INSERT INTO USER (NAME, PASSWORD, QQCOUNT, EXP) VALUES (\'" + userName + "', '" + passWord + +"', '" + qqCount + "', '" + expTime + "')";
	sql = "INSERT INTO user (NAME, PASSWORD, QQCOUNT, EXP, LEVEL, USERTYPE, referer, register_time) VALUES (\'" + \
		userName + "', '" + passWord + "', '" + qqCount + "', '" + expTime + "', '" + level + "', '" + userType + "', '" + referCode + "',NOW())";
	rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME user WHERE NAME= " << userName << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "1001";
		s_sqlPool.Recycle(pNode);
		return;
	}

	//获取当前用户id
	sql = "SELECT * from user WHERE NAME=\'" + userName + "\' and PASSWORD=\'" + passWord + "\'";
	rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << sql << "Failed" << mysql_error(pSqlCon) << END_LOGGER;
	}
	else
	{
		MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row)
		{
			//注册完后，需要新建一个自选股和条件
			char szSql[256] = { 0 };
			sprintf_s(szSql, sizeof(szSql) - 1, "INSERT INTO user_block (user_id, name, sort_num, visible, internal_flag, creation_time) VALUES (\
				%d ,'%s',%d,%d,%d,NOW())", atoi(row[0]), string2UTF8("自选股").c_str(), 0, 1, 1);
			sql = szSql;
			rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
			if(rc != 0)
			{
				WARNNING_LOGGER << sql << "Failed" << mysql_error(pSqlCon) << END_LOGGER;
			}

			sprintf_s(szSql, sizeof(szSql) - 1, "INSERT INTO user_block (user_id, name, sort_num, visible, internal_flag, creation_time) VALUES (\
				%d ,'%s',%d,%d,%d,NOW())", atoi(row[0]), string2UTF8("临时条件股").c_str(), 0, 1, 2);
			sql = szSql;
			rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
			if (rc != 0)
			{
				WARNNING_LOGGER << sql << "Failed" << mysql_error(pSqlCon) << END_LOGGER;
			}
		}
	}

	m_strResult = "1000";
	INFO_LOGGER << "账号注册成功: " << userName << END_LOGGER;

	s_sqlPool.Recycle(pNode);
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

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		WARNNING_LOGGER << "获取空闲节点失败！" << END_LOGGER;
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	std::string sql = "SELECT PASSWORD from user WHERE NAME=\'" + userName + "\'";
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME user WHERE NAME= " << userName << " Failed:" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "5001";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "5004";
		INFO_LOGGER << "用户: " << userName << " 不存在" << END_LOGGER;
		s_sqlPool.Recycle(pNode);
		return;
	}

	//比较库中的密码和传过来的密码
	std::string strData = row[0];
	if (strData.compare(oldPassWord) != 0)
	{
		INFO_LOGGER << "输入的旧密码不正确:" << userName << END_LOGGER;
		m_strResult = "5002";
		s_sqlPool.Recycle(pNode);
		return;
	}

	//更新密码
	sql = "UPDATE user SET PASSWORD=\'" + newPassWord + "\'" + \
		"WHERE NAME=\'" + userName + "\'";;
	rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		ERROR_LOGGER << "UPDATE user SET PASSWORD: " << userName << " Failed" << mysql_error(pSqlCon) << END_LOGGER;
		m_strResult = "5003";
	}
	m_strResult = "0";
	INFO_LOGGER << "RESET PASSWORD SUCCEED: " << userName << END_LOGGER;

	s_sqlPool.Recycle(pNode);
}

void CClientParse::GetUserLevel(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		WARNNING_LOGGER << "获取空闲节点失败！" << END_LOGGER;
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	std::string sql = "SELECT LEVEL from user WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row == nullptr || *row == nullptr)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "用户: " << m_userName << " 不存在" << END_LOGGER;
		s_sqlPool.Recycle(pNode);
		return;
	}

	//结果赋值
	m_strResult = row[0];
	INFO_LOGGER << "用户:" << m_userName << " 等级: " << m_strResult << END_LOGGER;

	s_sqlPool.Recycle(pNode);
}

void CClientParse::GetFunctionPermission(Document& doc)
{
	if (m_userName.empty() || atoi(m_userId.c_str()) <= 0)
	{
		m_strResult = "";
		return;
	}
	m_strResult = "";

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		WARNNING_LOGGER << "获取空闲节点失败！" << END_LOGGER;
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	//select uv.user_id,uv.version_id,v.name,vf.function_id,f.name,uv.expiry_time from user_version uv,version_function vf,function f,app_version v 
	//where uv.user_id=2 and uv.version_id=vf.version_id and f.id=vf.function_id and v.id=uv.version_id
	std::string sql = "select uv.user_id,uv.version_id,v.name,vf.function_id,f.name,uv.expiry_time from user_version uv,version_function vf,function f,app_version v \
		where uv.user_id=" + m_userId + " and uv.version_id=vf.version_id and f.id=vf.function_id and v.id=uv.version_id";
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "-1";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row;
	while (row = mysql_fetch_row(res))
	{
		m_strResult += row[3];
		m_strResult += ";";
	}

	s_sqlPool.Recycle(pNode);

	INFO_LOGGER << "用户:" << m_userName << " 函数控制权限为: " << m_strResult << END_LOGGER;
}

void CClientParse::BeyondDeadLine(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	SqlConNode *pNode = s_sqlPool.GetFree();
	if (!pNode)
	{
		return;
	}
	MYSQL *pSqlCon = pNode->pSqlCon;
	if (!pNode || !pSqlCon)
	{
		WARNNING_LOGGER << "获取数据库连接失败！" << END_LOGGER;
		return;
	}

	std::string sql = "SELECT EXP from user WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "3";
		s_sqlPool.Recycle(pNode);
		return;
	}
	MYSQL_RES *res = mysql_store_result(pSqlCon);//将结果保存在res结构体中
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "用户: " << m_userName << " 不存在" << END_LOGGER;
		s_sqlPool.Recycle(pNode);
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

	s_sqlPool.Recycle(pNode);
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

void CClientParse::DeleteSelfLoginState(CClientParse* pClient)
{
	CAutoLock lck(&s_clientLock);

	auto it = s_ClientMap.begin();
	for (; it != s_ClientMap.end(); it++)
	{
		if (it->second == pClient)
		{
			s_ClientMap.erase(it);
			break;
		}
	}
}

CLock CClientParse::s_clientLock;

CObjectPool<SqlConNode> CClientParse::s_sqlPool;

std::map<std::string, CClientParse*> CClientParse::s_ClientMap;

