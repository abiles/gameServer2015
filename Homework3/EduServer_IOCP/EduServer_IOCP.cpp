// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


__declspec(thread) int LThreadType = -1;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;


	if (false == GIocpManager->Initialize())
		return -1;

	if (false == GIocpManager->StartIoThreads())
		return -1;

	
	printf_s("Start Server\n");


	GIocpManager->StartAccept(); ///< block here...

	GIocpManager->Finalize();

	printf_s("End Server\n");

	delete GIocpManager;
	delete GSessionManager;
	delete GMemoryPool;

	return 0;
}

