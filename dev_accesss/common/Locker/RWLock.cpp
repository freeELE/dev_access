//#include "StdAfx.h"
#include "RWLock.h"

RWLock::RWLock(void)
{
#ifdef WIN32
	m_rwLock.eLockStatus = RWLOCK_IDLE; //����״̬
	m_rwLock.iRLockCount = 0; //��������
	m_rwLock.iRWaitingCount = 0; //���ȴ�����
	m_rwLock.iWWaitingCount = 0; //д�ȴ�����

	InitializeCriticalSection(&m_rwLock._lock);
	m_rwLock.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	//assert(m_rwLock.hEvent != INVALID_HANDLE_VALUE);
#else
	pthread_rwlock_init(&m_rwLock, NULL);
#endif
}

RWLock::~RWLock(void)
{
#ifdef WIN32
	DeleteCriticalSection(&m_rwLock._lock);
	CloseHandle(m_rwLock.hEvent);
#else
	pthread_rwlock_destroy(&m_rwLock);
#endif
}

void RWLock::lock_shared()
{
#ifdef WIN32
	bool isWaitReturn = false;
	while (1)
	{
		EnterCriticalSection(&m_rwLock._lock);
		if (isWaitReturn)
		{
			// �ȴ��¼�����,���¾�����
			m_rwLock.iRWaitingCount--;
		}

		if (m_rwLock.eLockStatus == RWLOCK_IDLE)
		{
			//����״̬��ֱ�ӵõ�����Ȩ
			m_rwLock.eLockStatus = RWLOCK_R;
			m_rwLock.iRLockCount++;
			LeaveCriticalSection(&m_rwLock._lock);
			break;
		}
		else if (m_rwLock.eLockStatus == RWLOCK_R)
		{
			if (m_rwLock.iWWaitingCount > 0)
			{
				//��д�����ڵȴ�,��һ��ȴ�,��ʹд���ܻ�þ�������
				m_rwLock.iRWaitingCount++;

				ResetEvent(m_rwLock.hEvent);
				LeaveCriticalSection(&m_rwLock._lock);

				WaitForSingleObject(m_rwLock.hEvent, INFINITE);

				//�ȴ����أ��������Լ���
				isWaitReturn = true;
			}
			else
			{
				//�õ�����������+1
				m_rwLock.iRLockCount++;
				LeaveCriticalSection(&m_rwLock._lock);
				break;
			}
		}
		else if (m_rwLock.eLockStatus == RWLOCK_W)
		{
			//�ȴ�д���ͷ�
			m_rwLock.iRWaitingCount++;

			ResetEvent(m_rwLock.hEvent);
			LeaveCriticalSection(&m_rwLock._lock);

			WaitForSingleObject(m_rwLock.hEvent, INFINITE);

			//�ȴ����أ��������Լ���
			isWaitReturn = true;
		}
		else
		{
			//assert(0);
			LeaveCriticalSection(&m_rwLock._lock);
			break;
		}
	}
#else
	pthread_rwlock_rdlock(&m_rwLock);
#endif
}

bool RWLock::try_lock_shared()
{
#ifdef WIN32
	bool bRet = false;
	EnterCriticalSection(&m_rwLock._lock);
	if (m_rwLock.eLockStatus == RWLOCK_IDLE)
	{
		//����״̬��ֱ�ӵõ�����Ȩ
		m_rwLock.eLockStatus = RWLOCK_R;
		m_rwLock.iRLockCount++;
		bRet = true;
	}
	else if (m_rwLock.eLockStatus == RWLOCK_R)
	{
		if (m_rwLock.iWWaitingCount > 0)
		{
		}
		else
		{
			//�õ�����������+1
			m_rwLock.iRLockCount++;
			bRet = true;
		}
	}
	LeaveCriticalSection(&m_rwLock._lock);
	return bRet;
#else
	return 0 == pthread_rwlock_tryrdlock(&m_rwLock);
#endif
}

void RWLock::unlock_shared()
{
	unlock();
}

void RWLock::lock_unique()
{
#ifdef WIN32
	bool isWaitReturn = false;
	while (1)
	{
		EnterCriticalSection(&m_rwLock._lock);
		if (isWaitReturn)
		{
			// �ȴ��¼�����,���¾�����
			m_rwLock.iWWaitingCount--;
		}

		if (m_rwLock.eLockStatus == RWLOCK_IDLE)
		{
			m_rwLock.eLockStatus = RWLOCK_W;
			LeaveCriticalSection(&m_rwLock._lock);
			break;
		}
		else 
		{
			m_rwLock.iWWaitingCount++;
			ResetEvent(m_rwLock.hEvent);
			LeaveCriticalSection(&m_rwLock._lock);

			WaitForSingleObject(m_rwLock.hEvent, INFINITE);

			//�ȴ����أ��������Լ���
			isWaitReturn = true;
		}
	}
#else
	pthread_rwlock_wrlock(&m_rwLock);
#endif
}

bool RWLock::try_lock_unique()
{
#ifdef WIN32
	bool bRet = false;
	EnterCriticalSection(&m_rwLock._lock);
	if (m_rwLock.eLockStatus == RWLOCK_IDLE)
	{
		m_rwLock.eLockStatus = RWLOCK_W;
		bRet = true;
	}
	LeaveCriticalSection(&m_rwLock._lock);;
	return bRet;
#else
	return 0 == pthread_rwlock_trywrlock(&m_rwLock);
#endif
}

void RWLock::unlock_unique()
{
	unlock();
}

void RWLock::unlock()
{
#ifdef WIN32
	EnterCriticalSection(&m_rwLock._lock);
	if (m_rwLock.iRLockCount > 0)
	{
		//��������
		m_rwLock.iRLockCount--;
		if (0 == m_rwLock.iRLockCount)
		{
			m_rwLock.eLockStatus = RWLOCK_IDLE;

			//�ͷ�
			if (m_rwLock.iWWaitingCount > 0 || m_rwLock.iRWaitingCount > 0)
			{
				//��ʱ�����������ڵȴ�,�����¼�ʹ�����¾�����
				SetEvent(m_rwLock.hEvent);
			}
			else
			{
				// ����
			}
		}
		else
		{
			//���ж���
		}
	}
	else
	{
		m_rwLock.eLockStatus = RWLOCK_IDLE;

		//д������
		if (m_rwLock.iWWaitingCount>0 || m_rwLock.iRWaitingCount>0)
		{
			//
			SetEvent(m_rwLock.hEvent);
		}
		else
		{
			//����
		}
	}
	LeaveCriticalSection(&m_rwLock._lock);
#else
	pthread_rwlock_unlock(&m_rwLock);
#endif
}
