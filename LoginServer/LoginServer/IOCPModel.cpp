#include "IOCPModel.h"
#include "logger.h"

// 每一个处理器上产生多少个线程
#define WORKER_THREADS_PER_PROCESSOR 2
// 同时投递的AcceptEx请求的数量
#define MAX_POST_ACCEPT              10
// 传递给Worker线程的退出信号
#define EXIT_CODE                    NULL


// 释放指针和句柄资源的宏

// 释放指针宏
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}
// 释放句柄宏
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
// 释放Socket宏
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { closesocket(x);x=INVALID_SOCKET;}}



CIOCPModel::CIOCPModel(void) :
	m_nThreads(0),
	m_hShutdownEvent(NULL),
	m_hIOCompletionPort(NULL),
	m_phWorkerThreads(NULL),
	m_strIP(DEFAULT_IP),
	m_nPort(DEFAULT_PORT),
	m_lpfnAcceptEx(NULL),
	m_pListenContext(NULL)
{
}


CIOCPModel::~CIOCPModel(void)
{
	// 确保资源彻底释放
	this->Stop();
}


/*********************************************************************
*函数功能：线程函数，根据GetQueuedCompletionStatus返回情况进行处理；
*函数参数：lpParam是THREADPARAMS_WORKER类型指针；
*函数说明：GetQueuedCompletionStatus正确返回时表示某操作已经完成，第二个参数lpNumberOfBytes表示本次套接字传输的字节数，
参数lpCompletionKey和lpOverlapped包含重要的信息，请查询MSDN文档；
*********************************************************************/
DWORD WINAPI CIOCPModel::_WorkerThread(LPVOID lpParam)
{
	THREADPARAMS_WORKER* pParam = (THREADPARAMS_WORKER*)lpParam;
	CIOCPModel* pIOCPModel = (CIOCPModel*)pParam->pIOCPModel;
	int nThreadNo = (int)pParam->nThreadNo;

	////INFO_LOGGER << "工作者线程启动，ID: " << nThreadNo << END_LOGGER;

	OVERLAPPED           *pOverlapped = NULL;
	PER_SOCKET_CONTEXT   *pSocketContext = NULL;
	DWORD                dwBytesTransfered = 0;

	//循环处理请求，直到接收到Shutdown信息为止
	while (WAIT_OBJECT_0 != WaitForSingleObject(pIOCPModel->m_hShutdownEvent, 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			pIOCPModel->m_hIOCompletionPort,
			&dwBytesTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);

		//接收EXIT_CODE退出标志，则直接退出
		if (EXIT_CODE == (DWORD)pSocketContext)
		{
			break;
		}

		//返回值为0，表示出错
		if (!bReturn)
		{
			DWORD dwErr = GetLastError();

			// 显示一下提示信息
			if (!pIOCPModel->HandleError(pSocketContext, dwErr))
			{
				break;
			}

			continue;
		}
		else
		{
			// 读取传入的参数
			PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT, m_Overlapped);

			// 判断是否有客户端断开了
			if ((0 == dwBytesTransfered) && (RECV_POSTED == pIoContext->m_OpType || SEND_POSTED == pIoContext->m_OpType))
			{
				//INFO_LOGGER << "客户端 " << inet_ntoa(pSocketContext->m_ClientAddr.sin_addr) << ":" << ntohs(pSocketContext->m_ClientAddr.sin_port) << " 断开连接." << END_LOGGER;

				ERROR_LOGGER << "客户端" <<  pIoContext->GetUserName() << "已退出!" << END_LOGGER;

				// 释放掉对应的资源
				pIOCPModel->_RemoveContext(pSocketContext);

				continue;
			}
			else
			{
				switch (pIoContext->m_OpType)
				{
					// Accept  
				case ACCEPT_POSTED:
				{
					pIoContext->m_nTotalBytes = dwBytesTransfered;
					pIOCPModel->_DoAccpet(pSocketContext, pIoContext);
				}
				break;

				// RECV
				case RECV_POSTED:
				{
					pIoContext->m_nTotalBytes = dwBytesTransfered;
					pIOCPModel->_DoRecv(pSocketContext, pIoContext);
				}
				break;

				case SEND_POSTED:
				{
					pIoContext->m_nSendBytes += dwBytesTransfered;
					if (pIoContext->m_nSendBytes < pIoContext->m_nTotalBytes)
					{
						//数据未能发送完，继续发送数据
						pIoContext->m_wsaBuf.buf = pIoContext->m_szBuffer + pIoContext->m_nSendBytes;
						pIoContext->m_wsaBuf.len = pIoContext->m_nTotalBytes - pIoContext->m_nSendBytes;
						pIOCPModel->PostWrite(pIoContext);
						//static int iCount = 1;
						//std::cout << "SEND_POSTED :" << iCount++ << std::endl;
					}
					else
					{
						pIOCPModel->PostRecv(pIoContext);
					}
				}
				break;
				default:
					// 不应该执行到这里
					ERROR_LOGGER << "_WorkThread中的 pIoContext->m_OpType 参数异常." << END_LOGGER;
					break;
				} //switch
			}//if
		}//if

	}//while

	//INFO_LOGGER << "工作者线程 " << nThreadNo << "号退出" << END_LOGGER;

	// 释放线程参数
	RELEASE(lpParam);

	return 0;
}

//函数功能：初始化套接字
bool CIOCPModel::LoadSocketLib()
{
	WSADATA wsaData;
	int nResult;
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 错误(一般都不可能出现)
	if (NO_ERROR != nResult)
	{
		ERROR_LOGGER << "初始化WinSock 2.2失败!" << END_LOGGER;
		return false;
	}

	return true;
}


//函数功能：启动服务器
bool CIOCPModel::Start()
{
	// 初始化线程互斥量
	InitializeCriticalSection(&m_csContextList);

	// 建立系统退出的事件通知
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 初始化IOCP
	if (false == _InitializeIOCP())
	{
		ERROR_LOGGER << "初始化IOCP失败！" << END_LOGGER;
		return false;
	}
	else
	{
		//INFO_LOGGER << "IOCP初始化完毕" << END_LOGGER;
	}

	// 初始化Socket
	if (false == _InitializeListenSocket())
	{
		ERROR_LOGGER << "Listen Socket初始化失败！" << END_LOGGER;
		this->_DeInitialize();
		return false;
	}
	else
	{
		//INFO_LOGGER << "Listen Socket初始化完毕." << END_LOGGER;
	}

	//INFO_LOGGER << "系统准备就绪，等候连接...." << END_LOGGER;

	return true;
}


////////////////////////////////////////////////////////////////////
//	开始发送系统退出消息，退出完成端口和线程资源
void CIOCPModel::Stop()
{
	if (m_pListenContext != NULL && m_pListenContext->m_Socket != INVALID_SOCKET)
	{
		// 激活关闭消息通知
		SetEvent(m_hShutdownEvent);

		for (int i = 0; i < m_nThreads; i++)
		{
			// 通知所有的完成端口操作退出
			PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
		}

		// 等待所有的客户端资源退出
		WaitForMultipleObjects(m_nThreads, m_phWorkerThreads, TRUE, INFINITE);

		// 清除客户端列表信息
		this->_ClearContextList();

		// 释放其他资源
		this->_DeInitialize();

		//INFO_LOGGER << "停止监听" << END_LOGGER;
	}
}


/*************************************************************
*函数功能：投递WSARecv请求；
*函数参数：
PER_IO_CONTEXT* pIoContext:	用于进行IO的套接字上的结构，主要为WSARecv参数和WSASend参数；
**************************************************************/
bool CIOCPModel::PostRecv(PER_IO_CONTEXT* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	pIoContext->ResetBuffer();
	pIoContext->m_OpType = RECV_POSTED;
	pIoContext->m_nSendBytes = 0;
	pIoContext->m_nTotalBytes = 0;

	// 初始化完成后，，投递WSARecv请求
	int nBytesRecv = WSARecv(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	int iErr = WSAGetLastError();
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != iErr))
	{
		ERROR_LOGGER << "投递第一个WSARecv失败[" << iErr << "]" << END_LOGGER;
		return false;
	}

	return true;
}

/*************************************************************
*函数功能：投递WSASend请求
*函数参数：
PER_IO_CONTEXT* pIoContext:	用于进行IO的套接字上的结构，主要为WSARecv参数和WSASend参数
*函数说明：调用PostWrite之前需要设置pIoContext中m_wsaBuf, m_nTotalBytes, m_nSendBytes；
**************************************************************/
bool CIOCPModel::PostWrite(PER_IO_CONTEXT* pIoContext)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwSendNumBytes = 0;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	//业务处理
	pIoContext->m_pClientParse->Run(p_wbuf->buf);
	pIoContext->m_pClientParse->GetReturnValue(pIoContext->m_wsaBuf.buf, MAX_BUFFER_LEN, pIoContext->m_wsaBuf.len);

	//更新总共需要发送的字节数
	pIoContext->m_nTotalBytes = pIoContext->m_wsaBuf.len;
	pIoContext->m_OpType = SEND_POSTED;

	//投递WSASend请求 -- 需要修改
	int nRet = WSASend(pIoContext->m_sockAccept, &pIoContext->m_wsaBuf, 1, &dwSendNumBytes, dwFlags,
		&pIoContext->m_Overlapped, NULL);

	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		ERROR_LOGGER << "投递WSASend失败！" << END_LOGGER;
		return false;
	}
	return true;
}


////////////////////////////////
// 初始化完成端口
bool CIOCPModel::_InitializeIOCP()
{
	// 建立第一个完成端口
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (NULL == m_hIOCompletionPort)
	{
		ERROR_LOGGER << "建立完成端口失败！错误代码: " << WSAGetLastError() << END_LOGGER;
		return false;
	}

	// 根据本机中的处理器数量，建立对应的线程数
	m_nThreads = WORKER_THREADS_PER_PROCESSOR * _GetNoOfProcessors();

	// 为工作者线程初始化句柄
	m_phWorkerThreads = new HANDLE[m_nThreads];

	// 根据计算出来的数量建立工作者线程
	DWORD nThreadID;
	for (int i = 0; i < m_nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->pIOCPModel = this;
		pThreadParams->nThreadNo = i + 1;
		m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, (void *)pThreadParams, 0, &nThreadID);
	}

	//INFO_LOGGER << "建立 _WorkerThread " << m_nThreads << "个" << END_LOGGER;

	return true;
}


/////////////////////////////////////////////////////////////////
// 初始化Socket
bool CIOCPModel::_InitializeListenSocket()
{
	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	// 服务器地址信息，用于绑定Socket
	struct sockaddr_in ServerAddress;

	// 生成用于监听的Socket的信息
	m_pListenContext = new PER_SOCKET_CONTEXT;

	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	m_pListenContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		ERROR_LOGGER << "初始化Socket失败，错误代码: " << WSAGetLastError() << END_LOGGER;
		return false;
	}
	else
	{
		ERROR_LOGGER << "WSASocket() 完成." << END_LOGGER;
	}

	// 将Listen Socket绑定至完成端口中
	if (NULL == CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket, m_hIOCompletionPort, (DWORD)m_pListenContext, 0))
	{
		ERROR_LOGGER << "绑定 Listen Socket至完成端口失败！错误代码: " << WSAGetLastError() << END_LOGGER;
		RELEASE_SOCKET(m_pListenContext->m_Socket);
		return false;
	}
	else
	{
		//INFO_LOGGER << "Listen Socket绑定完成端口 完成." << END_LOGGER;
	}

	// 填充地址信息
	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	// 这里可以绑定任何可用的IP地址，或者绑定一个指定的IP地址 
	//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);                      
	ServerAddress.sin_addr.s_addr = inet_addr(m_strIP.c_str());
	ServerAddress.sin_port = htons(m_nPort);

	// 绑定地址和端口
	if (SOCKET_ERROR == bind(m_pListenContext->m_Socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)))
	{
		ERROR_LOGGER << "bind()函数执行错误." << END_LOGGER;
		return false;
	}
	else
	{
		//INFO_LOGGER << "bind() 完成." << END_LOGGER;
	}

	// 开始进行监听
	if (SOCKET_ERROR == listen(m_pListenContext->m_Socket, SOMAXCONN))
	{
		ERROR_LOGGER << "Listen()函数执行出现错误." << END_LOGGER;
		return false;
	}
	else
	{
		//INFO_LOGGER << "Listen() 完成." << END_LOGGER;
	}

	// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
	// 所以需要额外获取一下函数的指针，
	// 获取AcceptEx函数指针
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		ERROR_LOGGER << "WSAIoctl 未能获取AcceptEx函数指针。错误代码: " << WSAGetLastError() << END_LOGGER;
		this->_DeInitialize();
		return false;
	}

	// 获取GetAcceptExSockAddrs函数指针，也是同理
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		ERROR_LOGGER << "WSAIoctl 未能获取GuidGetAcceptExSockAddrs函数指针。错误代码: " << WSAGetLastError() << END_LOGGER;
		this->_DeInitialize();
		return false;
	}


	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	//创建10个套接字，投递AcceptEx请求，即共有10个套接字进行accept操作；
	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// 新建一个IO_CONTEXT
		PER_IO_CONTEXT* pAcceptIoContext = m_pListenContext->GetNewIoContext();

		if (false == this->_PostAccept(pAcceptIoContext))
		{
			m_pListenContext->RemoveContext(pAcceptIoContext);
			return false;
		}
	}

	//INFO_LOGGER << "投递 " << MAX_POST_ACCEPT << " 个AcceptEx请求完毕" << END_LOGGER;

	return true;
}

////////////////////////////////////////////////////////////
//	最后释放掉所有资源
void CIOCPModel::_DeInitialize()
{
	// 删除客户端列表的互斥量
	DeleteCriticalSection(&m_csContextList);

	// 关闭系统退出事件句柄
	RELEASE_HANDLE(m_hShutdownEvent);

	// 释放工作者线程句柄指针
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreads[i]);
	}

	RELEASE(m_phWorkerThreads);

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_hIOCompletionPort);

	// 关闭监听Socket
	RELEASE(m_pListenContext);

	//INFO_LOGGER << "释放资源完毕" << END_LOGGER;
}


//====================================================================================
//
//				    投递完成端口请求
//
//====================================================================================


//////////////////////////////////////////////////////////////////
// 投递Accept请求
bool CIOCPModel::_PostAccept(PER_IO_CONTEXT* pAcceptIoContext)
{
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		ERROR_LOGGER << "INVALID_SOCKET == m_pListenContext->m_Socket, 数据为：" << pAcceptIoContext->m_wsaBuf.buf << END_LOGGER;
		return false;
	}

	// 准备参数
	DWORD dwBytes = 0;
	pAcceptIoContext->m_OpType = ACCEPT_POSTED;
	WSABUF *p_wbuf = &pAcceptIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pAcceptIoContext->m_Overlapped;

	// 为以后新连入的客户端先准备好Socket( 这个是与传统accept最大的区别 ) 
	pAcceptIoContext->m_sockAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoContext->m_sockAccept)
	{
		ERROR_LOGGER << "创建用于Accept的Socket失败！错误代码: " << WSAGetLastError() << END_LOGGER;
		return false;
	}

	//设置该套接字非阻塞
	unsigned long ul = 1;
	int nRet = ioctlsocket(pAcceptIoContext->m_sockAccept, FIONBIO, (unsigned long*)&ul);
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理，先记个日志
		ERROR_LOGGER << "设置套接字阻塞模式失败[" << WSAGetLastError() << "]" << END_LOGGER;
	}

	// 投递AcceptEx
	if (FALSE == m_lpfnAcceptEx(m_pListenContext->m_Socket, pAcceptIoContext->m_sockAccept, p_wbuf->buf, p_wbuf->len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, p_ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			ERROR_LOGGER << "投递 AcceptEx 请求失败，错误代码: " << WSAGetLastError() << END_LOGGER;
			return false;
		}
	}

	return true;
}


/********************************************************************
*函数功能：函数进行客户端接入处理；
*参数说明：
PER_SOCKET_CONTEXT* pSocketContext:	本次accept操作对应的套接字，该套接字所对应的数据结构；
PER_IO_CONTEXT* pIoContext:			本次accept操作对应的数据结构；
DWORD		dwIOSize:				本次操作数据实际传输的字节数
********************************************************************/
bool CIOCPModel::_DoAccpet(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext)
{

	if (pIoContext->m_nTotalBytes > 0)
	{
		//客户接入时，第一次接收dwIOSize字节数据
		_DoFirstRecvWithData(pIoContext);
	}
	else
	{
		//客户端接入时，没有发送数据，则投递WSARecv请求，接收数据
		_DoFirstRecvWithoutData(pIoContext);

	}

	// 5. 使用完毕之后，把Listen Socket的那个IoContext重置，然后准备投递新的AcceptEx
	pIoContext->ResetBuffer();
	return this->_PostAccept(pIoContext);
}


/////////////////////////////////////////////////////////////////
//函数功能：在有接收的数据到达的时候，进行处理
bool CIOCPModel::_DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext)
{
	//输出接收的数据
	SOCKADDR_IN* ClientAddr = &pSocketContext->m_ClientAddr;
	//INFO_LOGGER << "收到  " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息：" << pIoContext->m_wsaBuf.buf << END_LOGGER;
	//std::cout << "收到  " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息：" << pIoContext->m_wsaBuf.buf << std::endl;

	//static int iCount = 1;
	//std::cout << "_DoRecv :" << iCount++ << std::endl;

	//发送数据
	pIoContext->m_nSendBytes = 0;
	pIoContext->m_nTotalBytes = pIoContext->m_nTotalBytes;
	pIoContext->m_wsaBuf.len = pIoContext->m_nTotalBytes;
	pIoContext->m_wsaBuf.buf = pIoContext->m_szBuffer;
	return PostWrite(pIoContext);
}

/*************************************************************
*函数功能：AcceptEx接收客户连接成功，接收客户第一次发送的数据，故投递WSASend请求
*函数参数：
PER_IO_CONTEXT* pIoContext:	用于监听套接字上的操作
**************************************************************/
bool CIOCPModel::_DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext)
{
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);

	//1. 首先取得连入客户端的地址信息
	this->m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);

	//显示客户端信息
	//INFO_LOGGER << "客户端  " << inet_ntoa(ClientAddr->sin_addr) << ":" << ntohs(ClientAddr->sin_port) << " 信息：" << pIoContext->m_wsaBuf.buf << END_LOGGER;

	//2.为新接入的套接创建PER_SOCKET_CONTEXT，并将该套接字绑定到完成端口
	PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
	pNewSocketContext->m_Socket = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), ClientAddr, sizeof(SOCKADDR_IN));

	// 参数设置完毕，将这个Socket和完成端口绑定(这也是一个关键步骤)
	if (false == this->_AssociateWithIOCP(pNewSocketContext))
	{
		RELEASE(pNewSocketContext);
		return false;
	}

	//3. 继续，建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
	PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();
	pNewIoContext->m_OpType = SEND_POSTED;
	pNewIoContext->m_sockAccept = pNewSocketContext->m_Socket;
	pNewIoContext->m_nTotalBytes = pIoContext->m_nTotalBytes;
	pNewIoContext->m_nSendBytes = 0;
	pNewIoContext->m_wsaBuf.len = pIoContext->m_nTotalBytes;
	memcpy(pNewIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.buf, pIoContext->m_nTotalBytes);	//复制数据到WSASend函数的参数缓冲区

	//static int iCount = 1;
	//std::cout << "_DoFirstRecvWithData :" << iCount++ << std::endl;
	//此时是第一次接收数据成功，所以这里投递PostWrite，向客户端发送数据
	if (false == this->PostWrite(pNewIoContext))
	{
		pNewSocketContext->RemoveContext(pNewIoContext);
		return false;
	}

	//4. 如果投递成功，那么就把这个有效的客户端信息，加入到ContextList中去(需要统一管理，方便释放资源)
	this->_AddToContextList(pNewSocketContext);

	return true;
}

/*************************************************************
*函数功能：AcceptEx接收客户连接成功，此时并未接收到数据，故投递WSARecv请求
*函数参数：
PER_IO_CONTEXT* pIoContext:	用于监听套接字上的操作
**************************************************************/
bool CIOCPModel::_DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext)
{
	//为新接入的套接字创建PER_SOCKET_CONTEXT结构，并绑定到完成端口
	PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
	SOCKADDR_IN ClientAddr;
	int Len = sizeof(ClientAddr);

	getpeername(pIoContext->m_sockAccept, (sockaddr*)&ClientAddr, &Len);

	pNewSocketContext->m_Socket = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), &ClientAddr, sizeof(SOCKADDR_IN));

	//将该套接字绑定到完成端口
	if (false == this->_AssociateWithIOCP(pNewSocketContext))
	{
		RELEASE(pNewSocketContext);
		return false;
	}

	//投递WSARecv请求，接收数据
	PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();

	//此时是AcceptEx未接收到客户端第一次发送的数据，所以这里调用PostRecv，接收来自客户端的数据
	if (false == this->PostRecv(pNewIoContext))
	{
		pNewSocketContext->RemoveContext(pNewIoContext);
		return false;
	}

	//如果投递成功，那么就把这个有效的客户端信息，加入到ContextList中去(需要统一管理，方便释放资源)
	this->_AddToContextList(pNewSocketContext);

	return true;
}


/////////////////////////////////////////////////////
// 将句柄(Socket)绑定到完成端口中
bool CIOCPModel::_AssociateWithIOCP(PER_SOCKET_CONTEXT *pContext)
{
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pContext->m_Socket, m_hIOCompletionPort, (DWORD)pContext, 0);

	if (NULL == hTemp)
	{
		ERROR_LOGGER << "执行CreateIoCompletionPort()出现错误.错误代码：" << WSAGetLastError() << END_LOGGER;
		return false;
	}

	return true;
}



//////////////////////////////////////////////////////////////
// 将客户端的相关信息存储到数组中
void CIOCPModel::_AddToContextList(PER_SOCKET_CONTEXT *pHandleData)
{
	EnterCriticalSection(&m_csContextList);

	//m_arrayClientContext.Add(pHandleData);

	LeaveCriticalSection(&m_csContextList);
}

////////////////////////////////////////////////////////////////
//	移除某个特定的Context
void CIOCPModel::_RemoveContext(PER_SOCKET_CONTEXT *pSocketContext)
{
	EnterCriticalSection(&m_csContextList);

	for (auto it = m_arrayClientContext.begin(); it != m_arrayClientContext.end(); it++)
	{
		if (pSocketContext == *it)
		{
			RELEASE(pSocketContext);
			m_arrayClientContext.remove(pSocketContext);
			break;
		}
	}

	LeaveCriticalSection(&m_csContextList);
}

////////////////////////////////////////////////////////////////
// 清空客户端信息
void CIOCPModel::_ClearContextList()
{
	EnterCriticalSection(&m_csContextList);

	for (auto it = m_arrayClientContext.begin(); it != m_arrayClientContext.end(); it++)
	{
		delete *it;
	}
	m_arrayClientContext.clear();

	LeaveCriticalSection(&m_csContextList);
}



////////////////////////////////////////////////////////////////////
// 获得本机的IP地址
std::string CIOCPModel::GetLocalIP()
{
	// 获得本机主机名
	char hostname[MAX_PATH] = { 0 };
	gethostname(hostname, MAX_PATH);
	struct hostent FAR* lpHostEnt = gethostbyname(hostname);
	if (lpHostEnt == NULL)
	{
		return DEFAULT_IP;
	}

	// 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)
	LPSTR lpAddr = lpHostEnt->h_addr_list[0];

	// 将IP地址转化成字符串形式
	struct in_addr inAddr;
	memmove(&inAddr, lpAddr, 4);
	m_strIP = std::string(inet_ntoa(inAddr));

	return m_strIP;
}

///////////////////////////////////////////////////////////////////
// 获得本机中处理器的数量
int CIOCPModel::_GetNoOfProcessors()
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);

	return si.dwNumberOfProcessors;
}

/////////////////////////////////////////////////////////////////////
// 判断客户端Socket是否已经断开，否则在一个无效的Socket上投递WSARecv操作会出现异常
// 使用的方法是尝试向这个socket发送数据，判断这个socket调用的返回值
// 因为如果客户端网络异常断开(例如客户端崩溃或者拔掉网线等)的时候，服务器端是无法收到客户端断开的通知的

bool CIOCPModel::_IsSocketAlive(SOCKET s)
{
	int nByteSent = send(s, "", 0, 0);
	if (-1 == nByteSent)
		return false;
	return true;
}

///////////////////////////////////////////////////////////////////
//函数功能：显示并处理完成端口上的错误
bool CIOCPModel::HandleError(PER_SOCKET_CONTEXT *pContext, const DWORD& dwErr)
{
	// 如果是超时了，就再继续等吧  
	if (WAIT_TIMEOUT == dwErr)
	{
		// 确认客户端是否还活着...
		if (!_IsSocketAlive(pContext->m_Socket))
		{
			ERROR_LOGGER << "检测到客户端异常退出!" << END_LOGGER;
			this->_RemoveContext(pContext);
			return true;
		}
		else
		{
			//WARNNING_LOGGER << "网络操作超时！重试中..." << END_LOGGER;
			return true;
		}
	}

	// 可能是客户端异常退出了
	else if (ERROR_NETNAME_DELETED == dwErr)
	{
		ERROR_LOGGER << "客户端已退出!" << END_LOGGER;
		this->_RemoveContext(pContext);
		return true;
	}

	else
	{
		ERROR_LOGGER << "完成端口操作出现错误，线程退出。错误代码：" << (int)dwErr << END_LOGGER;
		return false;
	}
}




