#pragma once

#include "FastSpinlock.h"
#include "Timer.h"


class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable() : mLock(LO_BUSINESS_CLASS)
	{}
	virtual ~SyncExecutable() {}

	template <class R, class T, class... Args>
	R DoSync(R (T::*memfunc)(Args...), Args... args)
	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

		FastSpinlockGuard lockGuard(mLock);
		return (static_cast<T*>(this)->*memfunc)(args...);
	}
	

	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }

	
 	template <class T>
	std::shared_ptr<T> GetSharedFromThis()
 	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

 		return std::static_pointer_cast<T>(shared_from_this());
 	}

private:

	FastSpinlock mLock;
};


template <class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible<T, std::shared_ptr<SyncExecutable>>::value, "T should be shared_ptr SyncExecutable");

	auto bind = std::bind(memfunc, instance, std::forward<Args>(args)...);
	LTimer->PushTimerJob(std::static_pointer_cast<SyncExecutable>(instance), bind, after);
}