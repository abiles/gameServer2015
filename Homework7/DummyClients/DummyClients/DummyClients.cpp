// DummyClients.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DummyClients.h"
#include "Exception.h"
#include "DummyClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"
#include <time.h>
#include "MemoryPool.h"

/// config values
int MAX_CONNECTION = 0;
char CONNECT_ADDR[32] = { 0, }; 
unsigned short CONNECT_PORT = 0;


char* GetCommandOption(char** begin, char** end, const std::string& comparand)
{
	char** itr = std::find(begin, end, comparand);
	if (itr != end && ++itr != end)
		return *itr;
	
	return nullptr;
}


int _tmain(int argc, _TCHAR* argv[])
{
	srand((unsigned)time(NULL));
	LThreadType = THREAD_MAIN;

	MAX_CONNECTION = 10;
	strcpy_s(CONNECT_ADDR, "10.73.44.30");
	CONNECT_PORT = 41022;

	if ( argc < 2 )
	{
		printf_s("Usage Example: DummyClients --ip 10.73.44.30 --port 41022 --session 10\n");
	}
	else
	{

		char* ipAddr = GetCommandOption(argv, argv + argc, "--ip");
		char* port = GetCommandOption(argv, argv + argc, "--port");
		char* session = GetCommandOption(argv, argv + argc, "--session");

		if (ipAddr)
			strcpy_s(CONNECT_ADDR, ipAddr);
	
		if (port)
			CONNECT_PORT = atoi(port);
	
		if (session)
			MAX_CONNECTION = atoi(session);
		
	}
	
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

	printf_s("Start Dummies\n");

	if (false == GSessionManager->ConnectSessions())
		return -1;
	
	/// block here...
	getchar();

	printf_s("Terminating Dummies...\n");

	/// Disconnect request here
	GSessionManager->DisconnectSessions();

	GIocpManager->StopIoThreads();

	GSessionManager->PrintTotalTransferred();

	GIocpManager->Finalize();


	delete GIocpManager;
	delete GSessionManager;

	return 0;
}

