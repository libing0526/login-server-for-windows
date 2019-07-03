// LoginServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "IOCPModel.h"
#include "logger.h"

void CreateLog();
int main()
{
	CreateLog();

	CIOCPModel *pIocoModel = new CIOCPModel;

	if (false == pIocoModel->LoadSocketLib())
	{
		//INFO_LOGGER << "加载Winsock 2.2失败，服务器端无法运行！" << END_LOGGER;;
		return -1;
	}

	if (false == pIocoModel->Start())
	{
		//INFO_LOGGER << "服务器启动失败！" << END_LOGGER;;
		return -1;
	}

	//先弄个死循环
	while (1)
	{

	}

	pIocoModel->Stop();

	return 0;
}


void CreateLog()
{
	struct tm *newtime;
	char tmpbuf[128];
	time_t lt1;
	time(&lt1);
	newtime = localtime(&lt1);
	//::CreateDirectory(".\\log");
	CreateDirectory(TEXT(".\\log_c"), NULL);
	strftime(tmpbuf, 128, "%Y_%m_%d_log.txt", newtime);
	std::string filename(tmpbuf);
	//string logfile(".login_log");
	std::string full_logname(".\\log_c\\");
	//string current_logFile = //getenv("HOME") + full_logname + filename + logfile;
	//string full_logname = filename
	//string current_logFile(".\\log.txt");
	std::string current_logFile(full_logname + filename);
	INIT_LOGGER(current_logFile.c_str());

}