#ifndef _OFFER_OBJECT_POOL_H_
#define _OFFER_OBJECT_POOL_H_

#include "Lock.h"

/** 
* 对象池的封装，此处用来放数据库连接对象
*/
template<class T> class CObjectPool
{
public:
    /** 
    * 构造函数
    */
    CObjectPool()
    {
        m_pDataHead = NULL;
        m_pDataTail = NULL;
        m_pFreeHead = NULL;
        m_pFreeTail = NULL;
        m_iTotalNum = 0;
        m_iFreeNum = 0;
        m_iDataNum = 0;
        m_hPushEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	///创建加入事件
        m_hFreeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	///创建空闲事件

		//构造时，创建默认个数的节点
		Create();
    }
    
    /** 
    * 析构函数
    */
    virtual ~CObjectPool()
    {
		CAutoLock lck(&m_szListLock);

        Recycle(m_pDataHead);	///将数据节点放到空闲节点中。
        T* pNode = m_pFreeHead;
        ///循环释放节点
        while(m_pFreeHead)
        {
            m_pFreeHead = pNode->pNext;
            delete pNode;		/**< 释放节点 */
            pNode = m_pFreeHead;
        }
        
        m_pFreeTail = NULL;
        
        ///释放加入事件
        if(m_hPushEvent)
        {
            CloseHandle(m_hPushEvent);
            m_hPushEvent = 0;
        }
        
        ///释放空闲事件
        if(m_hFreeEvent)
        {
            CloseHandle(m_hFreeEvent);
            m_hFreeEvent = 0;
        }
    }
    
    /** 
    * 申请新节点
    * @param iCount: 需要申请的节点个数
    * @return 返回成功申请的节点个数
    */
    int Create(int iCount = 4)
    {
        ///检查参数
        if(iCount <= 0)
            return -1;
        
        T* pNode;
        int iLoop;
		CAutoLock lck(&m_szListLock);
        for(iLoop = 0; iLoop < iCount; iLoop ++)
        {
            pNode = new(std::nothrow) T();
            if(pNode == NULL)
                return iLoop;
            
            ///加入到队列中
            if(m_pFreeTail)
            {
                m_pFreeTail->pNext = pNode;
                m_pFreeTail = pNode;
            }
            else
            {
                m_pFreeHead = m_pFreeTail = pNode;
            }
            m_iTotalNum ++; 
            m_iFreeNum ++;
        }
        
        return iLoop;
    }
    
    /** 
    * 将数据入队列中
    * @param pNode: 节点数据指针
    * @return 0: 成功，其他失败
    */
    int Push(T* pNode)
    {
        ///检查传入的参数
        if(pNode == NULL)
            return -1;
        
		CAutoLock lck(&m_szListLock);
        if(m_pDataTail)
        {
            m_pDataTail->pNext = pNode;
            m_pDataTail = pNode;
        }
        else
        {
            m_pDataHead = m_pDataTail = pNode;
        }
        m_iDataNum ++;
        SetEvent(m_hPushEvent);
        
        return 0;
    }

    /** 
    * 从队列中取出节点
    * @param iCount: 需要取出的数据个数，默认为1
    * @return 节点头指针，当为空的时候队列中没有数据。
    */
    T* Pop(int iCount = 1)
    {
        if(iCount <= 0)
            return NULL;
        
        T* pNode = NULL; 
        T* pBefore = NULL;
        FBASE2_NAMESPACE::CAutoMutex cMutex(&m_szListLock);
        if(m_pDataHead)
        {
            pNode = m_pDataHead;
            for(int i = 0; i < iCount && m_pDataHead; i ++)
            {
                pBefore = m_pDataHead;
                m_pDataHead = m_pDataHead->pNext;
                m_iDataNum --;
            }
            if(!m_pDataHead)
            {
                m_pDataHead = m_pDataTail = NULL;
                ///如果出现空，就重置事件。
                ResetEvent(m_hPushEvent);
            }
            else
            {
                pBefore->pNext = NULL;
            }
        }
        
        return pNode;
    }
    
    /** 
    * 取出加入事件句柄
    * @return 事件句柄
    */
    HANDLE GetPushEvent() { return m_hPushEvent; }
    
    /** 
    * 取出空闲事件句柄
    * @return 事件句柄
    */
    HANDLE GetFreeEvent() { return m_hFreeEvent; }
    
    /** 
    * 取得一个空闲节点
    * @param iCount: 需要取得的空闲节点个数， 默认为1
    * @return 返回空闲节点队列
    */
    T* GetFree(int iCount = 1)
    {
        if(iCount <= 0 || !m_pFreeHead)
            return NULL;

		//如果没有空闲节点，那么等一下
		if (0 == m_iFreeNum)
		{
			//1000ms等不到空闲节点，那么返回
			if (WaitForSingleObject(m_hFreeEvent, 1000) != WAIT_OBJECT_0)
			{
				return NULL;
			}
		}

		//锁放在下面加，避免事件等待和触发在同一个锁内
		CAutoLock lck(&m_szListLock);
        
        T* pNode = m_pFreeHead;
        T* pBefore = m_pFreeHead;
        int i;
        for(i = 0; i < iCount && m_pFreeHead; i ++)
        {
            pBefore = m_pFreeHead;
            m_pFreeHead = m_pFreeHead->pNext;
            m_iFreeNum --;
        }
        if(!m_pFreeHead)
            m_pFreeTail = m_pFreeHead = NULL;
        else
            pBefore->pNext = NULL;
        
        return pNode;
    }
    
    /** 
    * 回收空闲节点
    * @param pNodeHead: 需要回收的空闲节点头
    * @return void
    */
    void Recycle(T* pNodeHead)
    {
		CAutoLock lck(&m_szListLock);

        if(!pNodeHead)
            return;
        
        if(m_pFreeTail)
        {
            m_pFreeTail->pNext = pNodeHead;
            m_pFreeTail = pNodeHead;
        }
        else
        {
            m_pFreeHead = m_pFreeTail = pNodeHead;
        }
        m_iFreeNum ++;
        while(m_pFreeTail->pNext)
        {
            m_pFreeTail = m_pFreeTail->pNext;
            m_iFreeNum ++;
        }
		//触发一下等待事件
		SetEvent(m_hFreeEvent);
    }
    
    /** 
    * 取统计信息
    * @param iTotalNum: 返回总的结节数
    * @param iDataNum: 返回数据节点数
    * @param iFreeNum: 返回空闲节点数
    * @return void
    */
    void GetStat(int &iTotalNum, int &iDataNum, int &iFreeNum)
    {
        iTotalNum = m_iTotalNum;
        iDataNum = m_iDataNum;
        iFreeNum = m_iFreeNum;
    }
    
private:
    T* m_pDataHead;
    T* m_pDataTail;
    T* m_pFreeHead;
    T* m_pFreeTail;/**< 首尾指针 */
    int m_iTotalNum, m_iDataNum, m_iFreeNum; /**< 节点数 */
    HANDLE m_hPushEvent;    /**< 有数据入队列事件 */
    HANDLE m_hFreeEvent;   /**< 空闲节点返回事件 */
    
	CLock m_szListLock;/**< 队列锁 */
};

#endif