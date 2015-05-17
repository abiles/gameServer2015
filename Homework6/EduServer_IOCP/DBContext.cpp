#include "stdafx.h"
#include "ThreadLocal.h"
#include "FastSpinlock.h"
#include "ClientSession.h"
#include "DBContext.h"


DatabaseJobContext::DatabaseJobContext(ClientSession* owner) : mSessionObject(owner), mSuccess(false)
{
	mSessionObject->AddRef();
}

DatabaseJobContext::~DatabaseJobContext()
{
	mSessionObject->ReleaseRef();
}

bool DatabaseJobContext::SQLExecute()
{
	//todo: �� �Լ��� �ݵ�� DB������Ǯ���� ����Ǿ�� �Ѵ�. �׷��� ������ CRASH��Ű��
	CRASH_ASSERT(LThreadType == THREAD_DB_WORKER);

	return OnSQLExecute();
}

void DatabaseJobContext::OnResult()
{
	//todo: �� �Լ��� �ݵ�� IO������Ǯ���� ����Ǿ�� �Ѵ�. �׷��� ������ CRASH��Ű��
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	if (mSuccess)
		OnSuccess();
	else
		OnFail();
}



