#include "stdafx.h"
#include "Exception.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"
#include "ThreadLocal.h"

FastSpinlock::FastSpinlock(const int lockOrder) : mLockFlag(0), mLockOrder(lockOrder)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
	if ( mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		// mLockFlag�� READ_MASK��� �����
		// mLockFlag�� 0�̶�� �����
		// mLockFlag�� WRITE_FLAG��� ��������
		// & LF_WRITE_MASK�� ���ִ� ������ READ ���¿����� ����ϰ� �ϱ� ����??
		// �� if���� ��� �ߴٸ� read�ϰ� �ִ� �ֵ��� �����鼭 ������ �ϴ� 1���� �ִ� ���
		// Ȥ�� read�� �ִ� ���� ������ �ϴ� �ְ� 1�� �ִ� ��� 

		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// �ٸ����� readlock Ǯ���ٶ����� ��ٸ���.
			// ���� ���ο� Write�� �ϱ� ���ؼ��� �а� �ִ� �ֵ鵵 ���� �� ���ľ���
			// �� while���� ���� �Ǳ� ���ؼ��� read�ϴ� ������ �������� �ϰ�
			// �׷��� read�ϴ� �ְ� �ִ� ���·� return �ȴ�
			// �� mLockFlog == LF_WRITE_FLAG�� ���´�
			while (mLockFlag & LF_READ_MASK)
				YieldProcessor();

			return;
		}

		// �ٷ� ���� �ִ� if���� �������� ���ϰ� ����� ����
		// if���� �����ϴ� ���� LF_WRITE_FLAG�� �ѹ� ������� ������
		// �׸�ŭ�� ���ش�
		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���?)
		// if ( readlock�� ������ )
			//return;
		// else
			// mLockFlag ����

		// WRITE_MAKS�ų� WRITE_FLAG���
		// if���� ������� ���Ѵ�.
		if (InterlockedAdd(&mLockFlag, 1) & LF_WRITE_MASK == 0)
		{
			return;
		}


		InterlockedAdd(&mLockFlag, -1);
		
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag ó�� 
	InterlockedAdd(&mLockFlag, -1);
	

	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}