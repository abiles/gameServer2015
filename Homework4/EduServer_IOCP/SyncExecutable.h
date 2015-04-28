#pragma once

#include "TypeTraits.h"
#include "FastSpinlock.h"
#include "Timer.h"


class SyncExecutable : public std::enable_shared_from_this<SyncExecutable>
{
public:
	SyncExecutable() : mLock(LO_BUSINESS_CLASS)
	{}
	virtual ~SyncExecutable() {}


	// player reset �κ��� dosync�� �̿��ؼ� �θ��� �ִµ�
	// �ٷ� mPlayer->reset���� ���� �ʴ� ������ �����ϱ�?
	// �� lock�� �ֱ��� 
	// lock���� ��ȣ�ϸ鼭 ���� �ϰ� ���� �༮�� ��쿡 
	// dosync �Լ��� �ҷ��� ó���ϴ� ���� 

	template <class R, class T, class... Args>
	R DoSync(R (T::*memfunc)(Args...), Args... args)
	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");

		
		//TODO: mLock���� ��ȣ�� ���¿���, memfunc�� �����ϰ� ����� R�� ����
		FastSpinlockGuard syncLock(mLock, true);

		
		return (static_cast<T*>(this)->*memfunc)(args...);
	
	}
	

	void EnterLock() { mLock.EnterWriteLock(); }
	void LeaveLock() { mLock.LeaveWriteLock(); }
	
	template <class T>
	std::shared_ptr<T> GetSharedFromThis()
	{
		static_assert(true == std::is_convertible<T, SyncExecutable>::value, "T should be derived from SyncExecutable");
		
		//TODO: this �����͸� std::shared_ptr<T>���·� ��ȯ.
		//(HINT: �� Ŭ������ std::enable_shared_from_this���� ��ӹ޾Ҵ�. �׸��� static_pointer_cast ���)

		return std::shared_ptr<T>(std::static_pointer_cast<T>(shared_from_this()));
		//return std::shared_ptr<T>((Player*)this); ///< �̷��� �ϸ� �ȵɰ�???

		///# �� �������� ���� ����? 
		/// �� �̷��� return std::static_pointer_cast<T>(shared_from_this());
	}

private:

	FastSpinlock mLock;
};


template <class T, class F, class... Args>
void DoSyncAfter(uint32_t after, T instance, F memfunc, Args&&... args)
{
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");
	static_assert(true == std::is_convertible<T, std::shared_ptr<SyncExecutable>>::value, "T should be shared_ptr SyncExecutable");

	//TODO: instance�� memfunc�� bind�� ��� LTimer->PushTimerJob() ����
	LTimer->PushTimerJob(instance, std::bind(memfunc, instance, std::forward<Args>(args)...), after);
	
}