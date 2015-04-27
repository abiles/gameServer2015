#pragma once
#include "Exception.h"
#include "TypeTraits.h"
#include "XTL.h"
#include "ThreadLocal.h"

class GrandCentralExecuter
{
public:
	typedef std::function<void()> GCETask;

	GrandCentralExecuter(): mRemainTaskCount(0)
	{}

	void DoDispatch(const GCETask& task)
	{
		CRASH_ASSERT(LThreadType == THREAD_IO_WORKER); ///< 일단 IO thread 전용

		
		if (InterlockedIncrement64(&mRemainTaskCount) > 1)
		{
			//TODO: 이미 누군가 작업중이면 어떻게?

			// lock-free 알고리즘을 생각해보면 
			// 하나가 실행하고 있으면 하나는 포기하니까
			return;
			
		}
		else
		{
			/// 처음 진입한 놈이 책임지고 다해주자 -.-;


			// mRemainTaskCount > 1 이것을 앞에서 체크한다
			// 그런데 Queue에 담아서 처리한다?
			// 무조건 1개의 task만이 if문을 통과하는 것이 보장이 안된다?
			// 무조건 1개면 while 없이 task만 실행해줘도 될것같은데
			// 왜일까?
			
			mCentralTaskQueue.push(task);
			
			while (true)
			{
				GCETask task;
				if (mCentralTaskQueue.try_pop(task))
				{
					//TODO: task를 수행하고 mRemainTaskCount를 하나 감소 
					// mRemainTaskCount가 0이면 break;
					task();
					--mRemainTaskCount;

					if (mRemainTaskCount <= 0)
						break;
				}
			}
		}

	}


private:
	typedef concurrency::concurrent_queue<GCETask, STLAllocator<GCETask>> CentralTaskQueue;
	CentralTaskQueue mCentralTaskQueue;
	int64_t mRemainTaskCount;
};

extern GrandCentralExecuter* GGrandCentralExecuter;



template <class T, class F, class... Args>
void GCEDispatch(T instance, F memfunc, Args&&... args)
{
	/// shared_ptr이 아닌 녀석은 받으면 안된다. 작업큐에 들어있는중에 없어질 수 있으니..
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");

	//TODO: intance의 memfunc를 std::bind로 묶어서 전달
	GGrandCentralExecuter->DoDispatch(std::bind(memfunc, instance, std::forward<Args>(args)...));

	//GGrandCentralExecuter->DoDispatch(bind);
}