#pragma once


class WorkerThread
{
public:
	WorkerThread(int workerThreadId, HANDLE hThread, HANDLE hCompletionPort);
	~WorkerThread();

	DWORD Run();

	bool DoIocpJob(); ///< false���� �Ǹ� ���Ḧ �ǹ�
	void DoSendJob();
	void DoTimerJob();

	HANDLE GetHandle() { return mThreadHandle;  }
	
private:

	HANDLE mThreadHandle;
	HANDLE mCompletionPort;
	int mWorkerThreadId;
};

