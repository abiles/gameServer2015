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
	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	if ( mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		// mLockFlag가 READ_MASK라면 통과함
		// mLockFlag가 0이라면 통과함
		// mLockFlag가 WRITE_FLAG라면 저지당함
		// & LF_WRITE_MASK를 해주는 이유는 READ 상태에서도 통과하게 하기 위해??
		// 이 if문을 통과 했다면 read하고 있는 애들이 있으면서 쓰려고 하는 1명이 있는 경우
		// 혹은 read할 애는 없고 쓰려고 하는 애가 1명 있는 경우 

		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			// 뭔가 새로운 Write를 하기 위해서는 읽고 있는 애들도 일을 다 마쳐야함
			// 이 while문이 종료 되기 위해서는 read하는 내용이 없어져야 하고
			// 그러면 read하는 애가 있는 상태로 return 된다
			// 즉 mLockFlog == LF_WRITE_FLAG인 상태다
			while (mLockFlag & LF_READ_MASK)
				YieldProcessor();

			return;
		}

		// 바로 위에 있는 if문을 만족하지 못하고 여기로 오면
		// if문을 진행하는 동안 LF_WRITE_FLAG를 한번 더해줬기 때문에
		// 그만큼을 빼준다
		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)
		// if ( readlock을 얻으면 )
			//return;
		// else
			// mLockFlag 원복

		// WRITE_MAKS거나 WRITE_FLAG라면
		// if문을 통과하지 못한다.
		if (InterlockedAdd(&mLockFlag, 1) & LF_WRITE_MASK == 0)
		{
			return;
		}


		InterlockedAdd(&mLockFlag, -1);
		
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag 처리 
	InterlockedAdd(&mLockFlag, -1);
	

	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}