/**********************************************************
原创作者：http://blog.csdn.net/piggyxp/article/details/6922277
修改时间：2013年1月11日22:45:10
**********************************************************/

#pragma once

#include <winsock2.h>
#include <MSWSock.h>
#include <vector>
#include <list>
#include <algorithm>
#include <string>
#include "ClientParse.h"

#pragma comment(lib,"ws2_32.lib")

//缓冲区长度 (1024*8)
#define MAX_BUFFER_LEN        8192  
//默认端口
#define DEFAULT_PORT          12345    
//默认IP地址
#define DEFAULT_IP            "127.0.0.1"


/****************************************************************
BOOL WINAPI GetQueuedCompletionStatus(
__in   HANDLE CompletionPort,
__out  LPDWORD lpNumberOfBytes,
__out  PULONG_PTR lpCompletionKey,
__out  LPOVERLAPPED *lpOverlapped,
__in   DWORD dwMilliseconds
);
lpCompletionKey [out] 对应于PER_SOCKET_CONTEXT结构，调用CreateIoCompletionPort绑定套接字到完成端口时传入；
A pointer to a variable that receives the completion key value associated with the file handle whose I/O operation has completed.
A completion key is a per-file key that is specified in a call to CreateIoCompletionPort.

lpOverlapped [out] 对应于PER_IO_CONTEXT结构，如：进行accept操作时，调用AcceptEx函数时传入；
A pointer to a variable that receives the address of the OVERLAPPED structure that was specified when the completed I/O operation was started.

****************************************************************/

// 在完成端口上投递的I/O操作的类型
typedef enum _OPERATION_TYPE
{
	ACCEPT_POSTED,                     // 标志投递的Accept操作
	SEND_POSTED,                       // 标志投递的是发送操作
	RECV_POSTED,                       // 标志投递的是接收操作
	NULL_POSTED                        // 用于初始化，无意义

}OPERATION_TYPE;


//每次套接字操作(如：AcceptEx, WSARecv, WSASend等)对应的数据结构：OVERLAPPED结构(标识本次操作)，关联的套接字，缓冲区，操作类型；
typedef struct _PER_IO_CONTEXT
{
	OVERLAPPED     m_Overlapped;                               // 每一个重叠网络操作的重叠结构(针对每一个Socket的每一个操作，都要有一个)              
	SOCKET         m_sockAccept;                               // 这个网络操作所使用的Socket
	WSABUF         m_wsaBuf;                                   // WSA类型的缓冲区，用于给重叠操作传参数的
	char           m_szBuffer[MAX_BUFFER_LEN];                 // 这个是WSABUF里具体存字符的缓冲区
	OPERATION_TYPE m_OpType;                                   // 标识网络操作的类型(对应上面的枚举)

	DWORD			m_nTotalBytes;	//数据总的字节数
	DWORD			m_nSendBytes;	//已经发送的字节数，如未发送数据则设置为0

	CClientParse   *m_pClientParse;	//客户端业务类

	//构造函数
	_PER_IO_CONTEXT()
	{
		ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
		m_sockAccept = INVALID_SOCKET;
		m_wsaBuf.buf = m_szBuffer;
		m_wsaBuf.len = MAX_BUFFER_LEN;
		m_OpType = NULL_POSTED;

		m_nTotalBytes = 0;
		m_nSendBytes = 0;
		m_pClientParse = new CClientParse();
	}
	//析构函数
	~_PER_IO_CONTEXT()
	{
		if (m_sockAccept != INVALID_SOCKET)
		{
			closesocket(m_sockAccept);
			m_sockAccept = INVALID_SOCKET;
		}
		if (m_pClientParse != NULL)
		{
			//清除自己的登录状态
			CClientParse::DeleteSelfLoginState(m_pClientParse);
			delete m_pClientParse;
			m_pClientParse = NULL;
		}
	}
	//重置缓冲区内容
	void ResetBuffer()
	{
		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
		m_wsaBuf.buf = m_szBuffer;
		m_wsaBuf.len = MAX_BUFFER_LEN;
	}

	//获取当前用户名
	std::string GetUserName()
	{
		if (m_pClientParse)
			return m_pClientParse->GetUserName();
		return "";
	}

} PER_IO_CONTEXT, *PPER_IO_CONTEXT;


//每个SOCKET对应的数据结构(调用GetQueuedCompletionStatus传入)：-
//SOCKET，该SOCKET对应的客户端地址，作用在该SOCKET操作集合(对应结构PER_IO_CONTEXT)；
typedef struct _PER_SOCKET_CONTEXT
{
	SOCKET      m_Socket;                                  //连接客户端的socket
	SOCKADDR_IN m_ClientAddr;                              //客户端地址
	std::list<_PER_IO_CONTEXT*> m_arrayIoContext;             //套接字操作，本例是WSARecv和WSASend共用一个PER_IO_CONTEXT

	//构造函数
	_PER_SOCKET_CONTEXT()
	{
		m_Socket = INVALID_SOCKET;
		memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
	}

	//析构函数
	~_PER_SOCKET_CONTEXT()
	{
		if (m_Socket != INVALID_SOCKET)
		{
			closesocket(m_Socket);
			m_Socket = INVALID_SOCKET;
		}
		// 释放掉所有的IO上下文数据
		for (auto pIoContext : m_arrayIoContext)
		{
			delete pIoContext;
		}
		m_arrayIoContext.clear();
	}

	//进行套接字操作时，调用此函数返回PER_IO_CONTEX结构
	_PER_IO_CONTEXT* GetNewIoContext()
	{
		_PER_IO_CONTEXT* p = new _PER_IO_CONTEXT;
		m_arrayIoContext.push_back(p);

		return p;
	}

	// 从数组中移除一个指定的IoContext
	void RemoveContext(_PER_IO_CONTEXT* pContext)
	{
		if (NULL == pContext)
			return;

		m_arrayIoContext.remove(pContext);
	}

} PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;



// 工作者线程的线程参数
class CIOCPModel;
typedef struct _tagThreadParams_WORKER
{
	CIOCPModel* pIOCPModel;                                   //类指针，用于调用类中的函数
	int         nThreadNo;                                    //线程编号

} THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;

// CIOCPModel类
class CIOCPModel
{
public:
	CIOCPModel(void);
	~CIOCPModel(void);

public:
	// 加载Socket库
	bool LoadSocketLib();
	// 卸载Socket库，彻底完事
	void UnloadSocketLib() { WSACleanup(); }

	// 启动服务器
	bool Start();
	//	停止服务器
	void Stop();

	// 获得本机的IP地址
	std::string GetLocalIP();

	// 设置监听端口
	void SetPort(const int& nPort) { m_nPort = nPort; }

	//投递WSASend，用于发送数据
	bool PostWrite(PER_IO_CONTEXT* pAcceptIoContext);

	//投递WSARecv用于接收数据
	bool PostRecv(PER_IO_CONTEXT* pIoContext);

protected:
	// 初始化IOCP
	bool _InitializeIOCP();
	// 初始化Socket
	bool _InitializeListenSocket();
	// 最后释放资源
	void _DeInitialize();

	//投递AcceptEx请求
	bool _PostAccept(PER_IO_CONTEXT* pAcceptIoContext);
	//在有客户端连入的时候，进行处理
	bool _DoAccpet(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);

	//连接成功时，根据第一次是否接收到来自客户端的数据进行调用
	bool _DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext);
	bool _DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext);
	//数据到达，数组存放在pIoContext参数中
	bool _DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);

	//将客户端socket的相关信息存储到数组中
	void _AddToContextList(PER_SOCKET_CONTEXT *pSocketContext);
	//将客户端socket的信息从数组中移除
	void _RemoveContext(PER_SOCKET_CONTEXT *pSocketContext);
	// 清空客户端socket信息
	void _ClearContextList();

	// 将句柄绑定到完成端口中
	bool _AssociateWithIOCP(PER_SOCKET_CONTEXT *pContext);

	// 处理完成端口上的错误
	bool HandleError(PER_SOCKET_CONTEXT *pContext, const DWORD& dwErr);

	//线程函数，为IOCP请求服务的工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	//获得本机的处理器数量
	int _GetNoOfProcessors();

	//判断客户端Socket是否已经断开
	bool _IsSocketAlive(SOCKET s);

private:

	HANDLE                       m_hShutdownEvent;              // 用来通知线程系统退出的事件，为了能够更好的退出线程

	HANDLE                       m_hIOCompletionPort;           // 完成端口的句柄

	HANDLE*                      m_phWorkerThreads;             // 工作者线程的句柄指针

	int		                     m_nThreads;                    // 生成的线程数量

	std::string                  m_strIP;                       // 服务器端的IP地址
	int                          m_nPort;                       // 服务器端的监听端口

	CRITICAL_SECTION             m_csContextList;               // 用于Worker线程同步的互斥量

	std::list<PER_SOCKET_CONTEXT*>  m_arrayClientContext;          // 客户端Socket的Context信息        

	PER_SOCKET_CONTEXT*          m_pListenContext;              // 用于监听的Socket的Context信息

	LPFN_ACCEPTEX                m_lpfnAcceptEx;                // AcceptEx 和 GetAcceptExSockaddrs 的函数指针，用于调用这两个扩展函数
	LPFN_GETACCEPTEXSOCKADDRS    m_lpfnGetAcceptExSockAddrs;
};

