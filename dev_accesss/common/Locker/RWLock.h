#ifndef __RW_LOCK_H__
#define __RW_LOCK_H__

#ifdef WIN32
#pragma once
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef WIN32
typedef struct _tag_RWLock
{
	int eLockStatus; //����״̬
	int iRLockCount; //��������
	int iRWaitingCount; //���ȴ�����
	int iWWaitingCount; //д�ȴ�����

	HANDLE hEvent; //֪ͨ�¼�
	CRITICAL_SECTION _lock; //�ٽ��� <<�˴������Ƿ��û�����>>
}_RWLock;
#endif

class AutoRLock; // �����ǵݹ���,����һ���������ظ�����.
class AutoWLock;// д�����ǵݹ���.���һ���߳����ظ�����,�ᵼ�³�����ѭ��..
class RWLock
{
	enum RWLockStatus
	{
		RWLOCK_IDLE = 0, //����
		RWLOCK_R, //����
		RWLOCK_W, //д��
	};

	friend class AutoRLock;
	friend class AutoWLock;

public:
	RWLock(void);
	~RWLock(void);

private:
	void lock_shared();
	bool try_lock_shared();
	void unlock_shared();

	void lock_unique();
	bool try_lock_unique();
	void unlock_unique();

	void unlock();

private:
#ifdef WIN32
	_RWLock m_rwLock;
#else
	pthread_rwlock_t m_rwLock;
#endif
};

class AutoRLock
{
public:
	AutoRLock(RWLock &rwLock, bool bTry=false)
		: m_rwLock(rwLock)
	{
		if (bTry)
		{
			m_bIsLock = m_rwLock.try_lock_shared();
		}
		else
		{
			m_rwLock.lock_shared();
			m_bIsLock = true;
		}
	}

	~AutoRLock()
	{
		if (m_bIsLock)
			m_rwLock.unlock_shared();
	}

	bool IsLock()
	{
		return m_bIsLock;
	}

private:
	RWLock &m_rwLock;
	bool   m_bIsLock;
};

class AutoWLock
{
public:
	AutoWLock(RWLock &rwLock, bool bTry=false)
		: m_rwLock(rwLock)
	{
		if (bTry)
		{
			m_bIsLock = m_rwLock.try_lock_unique();
		}
		else
		{
			m_rwLock.lock_unique();
			m_bIsLock = true;
		}
	}

	~AutoWLock()
	{
		if (m_bIsLock)
			m_rwLock.unlock_unique();
	}

	bool IsLock()
	{
		return m_bIsLock;
	}
private:
	RWLock &m_rwLock;
	bool   m_bIsLock;
};


#endif //__RW_LOCK_H__
