#include "CLoginVerify.h"
#include "logger.h"
#include "cs_const.h"
#include "md5.h"
using namespace std;
#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#pragma comment(lib, "json_d.lib")
#else
#pragma comment(lib, "json.lib")        
#endif

CLoginVerify::CLoginVerify()
{
    _keepAlive = 0;
}
void CLoginVerify::CreateLog()
{
    struct tm *newtime;
    char tmpbuf[128];
    time_t lt1;
    time(&lt1);
    newtime=localtime(&lt1);
    CreateDirectory(TEXT(".\\log_c"),NULL);
    strftime(tmpbuf, 128, "%Y_%m_%d_log.txt", newtime);
    string filename(tmpbuf);
    //string logfile(".login_log");
    string full_logname(".\\log_c\\");
    //string current_logFile = //getenv("HOME") + full_logname + filename + logfile;
    //string full_logname = filename
    //string current_logFile(".\\log.txt");
    string current_logFile(full_logname + filename);
    INIT_LOGGER(current_logFile.c_str());
    INFO_LOGGER << "INFO_LOGGER process param!--------------------------------------" << END_LOGGER;
    INFO_LOGGER << "LOGGER: creat logFile at " << " " << current_logFile << END_LOGGER;
}
// 初始化env
int CLoginVerify::InitEnv(Json::Value& serverInfo)
{
    CreateLog();
    //加载套接字  
    WSADATA wsaData;
    //char buff[1024];
    //memset(buff, 0, sizeof(buff));
    int errorCode = 0;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        errorCode = WSAGetLastError();
        ERROR_LOGGER << "LOGGER: Failed to load Winsock: " << errorCode << END_LOGGER;
        return errorCode;
    }

    SOCKADDR_IN addrSrv;
    addrSrv.sin_family = AF_INET;
    int port = serverInfo["port"].asInt();
    addrSrv.sin_port = htons(port);
    string serverIp = serverInfo["serverIp"].asString();
    addrSrv.sin_addr.S_un.S_addr = inet_addr(serverIp.c_str());

    //创建套接字  
    _clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET_ERROR == _clientSocket)
    {
        errorCode = WSAGetLastError();
        ERROR_LOGGER << "Create Socket failed: " << errorCode << END_LOGGER;
        return errorCode;
    }

    //向服务器发出连接请求  
    if (connect(_clientSocket, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET){
        errorCode = WSAGetLastError();
        ERROR_LOGGER << "Connect failed: " << errorCode << END_LOGGER;
        return errorCode;
    }
    return 0;
}

int CLoginVerify::Login(Json::Value& userInfo)
{
    //UserInfo userInfo = { "weitao", "701899", 1234 };
    //send(sockClient, sendBuff, sizeof(sendBuff), 0);
    userInfo["type"] = CS_LOGIN;
    Json::Value tmpValue = userInfo;
    string passWord = tmpValue["passWord"].asCString();
    //std::string src = "123456";
    Md5Encode encodeObj;
    std::string encodePassWord = encodeObj.Encode(passWord);
    tmpValue["passWord"] = encodePassWord;
    string sendMes = tmpValue.toStyledString();

	//设置超时
	int timeOver = 5000;
	::setsockopt(_clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeOver, sizeof(timeOver));

    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Login send failed: " << nErrCode << END_LOGGER;
        return nErrCode;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024,0);
    if (ret < 0) 
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Login Recv failed: " << WSAGetLastError() << END_LOGGER;
        return nErrCode;
    }
    int returnCode = atoi(buff);
    INFO_LOGGER << "Login return code: " << returnCode << END_LOGGER;
    return returnCode;
}

// 用户注册
int CLoginVerify::Register(Json::Value& userInfo)
{
    userInfo["type"] = CS_REGISTER;
    Json::Value tmpValue = userInfo;
    string passWord = tmpValue["passWord"].asCString();
    //std::string src = "123456";
    Md5Encode encodeObj;
    std::string encodePassWord = encodeObj.Encode(passWord);
    tmpValue["passWord"] = encodePassWord;
    string sendMes = tmpValue.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Register send failed: " << nErrCode << END_LOGGER;
        return nErrCode;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0)
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Register Recv failed: " << nErrCode << END_LOGGER;
        return nErrCode;
    }
    else
    {
        int returnCode = atoi(buff);
        INFO_LOGGER << "return code: " << returnCode << END_LOGGER;
        return returnCode;
    }
}

// 获取最新版本客户端信息
const std::string CLoginVerify::GetLatestVersion()
{
    Json::Value userInfo;
    userInfo["type"] = CS_GETLASTESTVER;
    string sendMes = userInfo.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) {
        ERROR_LOGGER << "GetLatestVersion send failed: " << WSAGetLastError() << END_LOGGER;
        return NULL;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "GetLatestVersion recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    INFO_LOGGER <<"查询到最新版本: " << buff << END_LOGGER;
    return buff;
}

// 获取用户截止日期
const std::string CLoginVerify::GetDeadLine()
{
    Json::Value userInfo;
    userInfo["type"] = CS_GETDEADLINE;
    string sendMes = userInfo.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) {
        ERROR_LOGGER << "GetDeadLine send failed: " << WSAGetLastError() << END_LOGGER;
        return NULL;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "GetDeadLine recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    INFO_LOGGER <<"查询到截止日期: " << buff << END_LOGGER;
    return buff;
}

const std::string CLoginVerify::GetSystemTime()
{
    Json::Value userInfo;
    userInfo["type"] = CS_GETSYSTEMTIME;
    string sendMes = userInfo.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        ERROR_LOGGER << "GetSystemTime send failed: " << WSAGetLastError() << END_LOGGER;
        return NULL;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "GetSystemTime recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    INFO_LOGGER <<"查询到系统日期: " << buff << END_LOGGER;
    return buff;
}

bool CLoginVerify::BeyondDeadLine()
{
    Json::Value userInfo;
    userInfo["type"] = CS_BEYONDDEADLINE;
    string sendMes = userInfo.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        ERROR_LOGGER << "BeyondDeadLine send failed: " << WSAGetLastError() << END_LOGGER;
        return NULL;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "BeyondDeadLine recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    INFO_LOGGER <<"是否超出截止日期: " << buff << END_LOGGER;
    int nResult =  atoi(buff);
    if (nResult == 0)
    {
        return false;
    }
    return true;
}
// 获取用户类型
USER_TYPE CLoginVerify::GetUserType()
{
    Json::Value userInfo;
    userInfo["type"] = CS_GETUSERTYPE;
    string sendMes = userInfo.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        ERROR_LOGGER << "GetUserType send failed: " << WSAGetLastError() << END_LOGGER;
        return USER_TYPE::NON;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "GetUserType recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    int nResult = atoi(buff);
    if (nResult == 1)
    {
        INFO_LOGGER << _userName << " 用户类型: 股票" << END_LOGGER;
        return USER_TYPE::STOCK;
    }
    else if(nResult == 2)
    {
        INFO_LOGGER << _userName << " 用户类型: 期货" << END_LOGGER;
        return USER_TYPE::FUTURES;
    }
    return USER_TYPE::NON;
}
// 获取用户等级
USER_LEVEL CLoginVerify::GetUserLevel()
{
    Json::Value userInfo;
    userInfo["type"] = CS_GETUSERLEVEL;
    string sendMes = userInfo.toStyledString();

    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        ERROR_LOGGER << "GetUserLevel send failed: " << WSAGetLastError() << END_LOGGER;
        return USER_LEVEL::NONE;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0) 
    {
        ERROR_LOGGER << "GetUserLevel recv failed: " << WSAGetLastError() << END_LOGGER;
    }
    int nResult =  atoi(buff);
    INFO_LOGGER << _userName << " 用户等级: " << buff << END_LOGGER;
    if (nResult == 1)
    {
        return USER_LEVEL::PRIMARY;
    }
    else if(nResult == 2)
    {
        return USER_LEVEL::INTERMEDIATE;
    }
    else if (nResult == 3)
    {
        return USER_LEVEL::SENIOR;
    }
    return USER_LEVEL::NONE;
}

// 设置用户截止日期
int CLoginVerify::SetDeadLine(const Json::Value& sendDeadLine)
{
    return 0;
}

// 清理env
void CLoginVerify::cleanEnv()
{
    if (_clientSocket != NULL)
    {
        closesocket(_clientSocket);
        WSACleanup();
    }
    return;
}

SOCKET CLoginVerify::GetSocket()
{
    return _clientSocket;
}

void CLoginVerify::SetSocket(SOCKET sock)
{
    _clientSocket = sock;
}
DWORD WINAPI CreateKeepThread(LPVOID lpParameter)
{
    //AliveStr* tmpParam = (AliveStr*)lpParameter;
    return 0;
    CLoginVerify* tmpParam = (CLoginVerify*)lpParameter;
    BOOL bNoDelay = TRUE;
    ::setsockopt(tmpParam->GetSocket(), IPPROTO_TCP, TCP_NODELAY , (char FAR *)&bNoDelay, sizeof(BOOL));
    while(1)
    {
        Sleep(10000);
        if (tmpParam->GetSocket() == NULL)
        {
            return 0;
        }
        if (tmpParam->_keepAlive == 1)
        {
            return 0;
        }
        Json::Value userInfo;
        userInfo["type"] = 100;
        string sendMes = userInfo.toStyledString();
        int nRet = send(tmpParam->GetSocket(), sendMes.c_str(), sendMes.length(), 0);
        if (nRet < 0)
        {
            int errCode = WSAGetLastError();
            INFO_LOGGER << "网络异常: " << errCode << END_LOGGER;
            tmpParam->_keepAlive == 2;
            closesocket(tmpParam->GetSocket());
            tmpParam->SetSocket(NULL);
            return 0;
            //if (errCode == 10053)
            //{
            //    tmpParam->_keepAlive = 1;
            //    INFO_LOGGER << "其他用户已登录: " << errCode << END_LOGGER;
            //    //closesocket(tmpParam->_clientSocket);
            //    tmpParam->SetSocket(NULL);
            //}
            //else
            //{
            //    tmpParam->_keepAlive = 2;
            //    INFO_LOGGER << "网络异常: " << errCode << END_LOGGER;
            //    //shutdown(tmpParam->_clientSocket, SD_BOTH);//10053
            //    tmpParam->SetSocket(NULL);
            //}
            //WSACleanup();
            //closesocket(tmpParam->_clientSocket);
            //return 0;
        }
        /*char buff[1024] = { 0 };
        int ret = recv(tmpParam->_clientSocket, buff, 1024, 0);
        nRet = atoi(buff);
        if (nRet == 1)
        {
        tmpParam->_keepAlive = 1;
        INFO_LOGGER << "其他用户已登录" << END_LOGGER;
        return 0;
        }*/
    }
}
DWORD WINAPI CreateReLoginThread(LPVOID lpParameter)
{
    CLoginVerify* tmpParam = (CLoginVerify*)lpParameter;
    BOOL bNoDelay = TRUE;
    ::setsockopt(tmpParam->GetSocket(), IPPROTO_TCP, TCP_NODELAY , (char FAR *)&bNoDelay, sizeof(BOOL));

    Json::Value userInfo;
    userInfo["type"] = CS_RELOGINTHREAD;
    string sendMes = userInfo.toStyledString();
    while(1)
    {
        //Sleep(3000);
        Sleep(5000);
        if (tmpParam->GetSocket() == NULL)
        {
            return 0;
        }
        if (tmpParam->_keepAlive == 2)
        {
            return 0;
        }
        int nRet = send(tmpParam->GetSocket(), sendMes.c_str(), sendMes.length(), 0);
        if (nRet <= 0)
        {
            tmpParam->_keepAlive = 2;
            closesocket(tmpParam->GetSocket());
            tmpParam->SetSocket(NULL);
            INFO_LOGGER << "1 检测到网络异常，当前用户退出!" << END_LOGGER;
            cout << "1 检测到网络异常，当前用户退出!" << endl;
            return 0;
        }
        char buff[1024] = { 0 };
        struct timeval tv_out;
        tv_out.tv_sec = 8;
        tv_out.tv_usec = 0;
        int timeOver = 1000;
        ::setsockopt(tmpParam->GetSocket(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeOver, sizeof(timeOver));
        nRet = recv(tmpParam->GetSocket(), buff, 1024, 0);
        if (nRet == -1)
        {
            //tmpParam->_keepAlive = 2;
            //closesocket(tmpParam->GetSocket());
            //tmpParam->SetSocket(NULL);
            //INFO_LOGGER << "2 检测到网络异常，当前用户退出!" << END_LOGGER;
            //cout << "2 检测到网络异常，当前用户退出!" << endl;
            //return 0;
        }
        nRet = atoi(buff);
        cout << "收到server info: "<< nRet << endl;

        if (nRet == 1)
        {
            tmpParam->_keepAlive = 1;
            closesocket(tmpParam->GetSocket());
            tmpParam->SetSocket(NULL);
            INFO_LOGGER << "检测到用户重复登陆，当前用户退出!" << END_LOGGER;
            cout << "检测到用户重复登陆，当前用户退出!" << endl;
            return 0;
        }
    }
    return 0;
}

void CLoginVerify::KeepAlive()
{
    _keepAlive = 0;
    HANDLE hThread = ::CreateThread(NULL, 0,  CreateReLoginThread, (LPVOID)this, 0, NULL);
    HANDLE hThread2 = ::CreateThread(NULL, 0, CreateKeepThread, (LPVOID)this, 0, NULL);
    if (hThread == NULL)
    {
        ERROR_LOGGER << "Failed to create a new thread! Error code: " << WSAGetLastError() << END_LOGGER;
        ::WSACleanup();
        //exit(1);
    }
    ::CloseHandle(hThread);
    ::CloseHandle(hThread2);
}

int CLoginVerify::ResetPassWord(Json::Value& newPassWd)
{
    newPassWd["type"] = CS_RESETPASSWORD;
    string oldPassWord = newPassWd["oldPassWord"].asCString();
    //std::string src = "123456";
    Md5Encode encodeObj;
    std::string encodePassWord = encodeObj.Encode(oldPassWord);
    newPassWd["oldPassWord"] = encodePassWord;

    string newPassWord = newPassWd["newPassWord"].asCString();
    //std::string src = "123456";
    //Md5Encode encodeObj;
    encodePassWord = encodeObj.Encode(newPassWord);
    newPassWd["newPassWord"] = encodePassWord;

    string sendMes = newPassWd.toStyledString();
    int nRet = send(_clientSocket, sendMes.c_str(), sendMes.length(), 0);
    if (nRet != sendMes.length()) 
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Reset PassWord send failed: " << nErrCode << END_LOGGER;
        return nErrCode;
    }
    char buff[1024] = { 0 };
    int ret = recv(_clientSocket, buff, 1024, 0);
    if (ret < 0)
    {
        int nErrCode = WSAGetLastError();
        ERROR_LOGGER << "Reset PassWord Recv failed: " << nErrCode << END_LOGGER;
        return nErrCode;
    }
    else
    {
        int returnCode = atoi(buff);
        INFO_LOGGER << "Reset PassWord return code: " << returnCode << END_LOGGER;
        return returnCode;
    }
}