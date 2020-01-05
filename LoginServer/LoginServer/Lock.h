/*******************************************************************************
*  @file      
*  @author    
*  @brief     智能锁的简单封装
******************************************************************************/

#ifndef __LOCK_H__
#define __LOCK_H__

#include <windows.h>

class CFastLock
{
public:
	CFastLock();
	~CFastLock();

public:
#ifdef _MSC_VER
	CRITICAL_SECTION m_critical_section;
#else
	pthread_mutex_t m_mutex;
#endif
};


class CLock
{
public:
    void lock();
    void unlock();
#ifndef _MSC_VER
    virtual bool try_lock();
#endif
private:
	CFastLock	m_lock;
};

class CAutoLock
{
public:
    CAutoLock(CLock* pLock);
    virtual ~CAutoLock();
private:
    CLock* m_pLock;
};

#endif
