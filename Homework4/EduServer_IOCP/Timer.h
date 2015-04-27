#pragma once
#include "XTL.h"

class SyncExecutable;

// TimerTask가 함수 객체인 이유는 std::bind로 전달받은 함수객체를 이용하기 위해서
typedef std::function<void()> TimerTask;
typedef std::shared_ptr<SyncExecutable> SyncExecutablePtr;



struct TimerJobElement
{
	TimerJobElement(SyncExecutablePtr owner, const TimerTask& task, int64_t execTick)
	: mOwner(owner), mTask(task), mExecutionTick(execTick)
	{}

	virtual ~TimerJobElement() {}

	SyncExecutablePtr mOwner;
	TimerTask	mTask;
	int64_t		mExecutionTick;
};


// priority queue의 비교 방식을 정의 
struct TimerJobComparator
{
	bool operator()(const TimerJobElement& lhs, const TimerJobElement& rhs)
	{
		return lhs.mExecutionTick > rhs.mExecutionTick;
	}
};

// 사용할 타입, 저장 방식, 비교 방식
typedef std::priority_queue<TimerJobElement, std::vector<TimerJobElement, STLAllocator<TimerJobElement> >, TimerJobComparator> TimerJobPriorityQueue;

class Timer
{
public:

	Timer();

	void PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after);

	void DoTimerJob();

private:

	TimerJobPriorityQueue	mTimerJobQueue;

};

