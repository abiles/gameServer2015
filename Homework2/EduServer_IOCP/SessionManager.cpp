#include "stdafx.h"
#include "FastSpinlock.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


SessionManager* GSessionManager = nullptr;

SessionManager::~SessionManager()
{
	for (auto it : mFreeSessionList)
	{
		delete it;
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = new ClientSession();
			
		mFreeSessionList.push_back(client);
	}
}





void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	client->SessionReset();

	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
}

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
	{
		//TODO mFreeSessionList���� ClientSession* ������ PostAccept() ���ֱ�.. (���� ReturnClientSession�� ���� �ݴ�� �ϸ� �� ��?)
		
		// AddRef()�� �翬�� ����� �ϰ�...

		// ���н� false
		//if (false == newClient->PostAccept())
		//	return false;
		ClientSession* pCurrentClient = mFreeSessionList.front();
		mFreeSessionList.pop_front();
		++mCurrentIssueCount;

		if (false == pCurrentClient->PostAccept())
			return false;
		
		pCurrentClient->AddRef();

	}


	return true;
}