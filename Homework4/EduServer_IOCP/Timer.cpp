#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	//TODO: mTimerJobQueue에 TimerJobElement를 push..
	mTimerJobQueue.push(TimerJobElement(owner, task, after));
	
	// 아래에 이걸 써먹는 방식이 있음
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		// 우선순위가 가장 높은 녀석을 꺼내서 해당 task를 하게 함
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		timerJobElem.mOwner->EnterLock();
		
		timerJobElem.mTask();

		timerJobElem.mOwner->LeaveLock();

		mTimerJobQueue.pop();
	}


}

