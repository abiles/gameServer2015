#include "stdafx.h"
#include "Exception.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	INFINITE //20

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;
LPFN_ACCEPTEX GAcceptEX = nullptr;
GUID GGUIDAcceptEx = WSAID_ACCEPTEX;

LPFN_DISCONNECTEX GDisconnectEX = nullptr;
GUID GGUIDDisconnectEx = WSAID_DISCONNECTEX;


//TODO AcceptEx DisconnectEx �Լ� ����� �� �ֵ��� ����.

BOOL NewDisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved)
{
	if (GDisconnectEX == nullptr)
		return FALSE;

	return GDisconnectEX(hSocket, lpOverlapped, dwFlags, reserved);
	
}

// ����: �ֽ� ������ Windows SDK������ �׳� �����Ǿ� ����
BOOL NewAcceptEx(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
	DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped)
{
	if (GAcceptEX == nullptr)
		return FALSE;
	
	return GAcceptEX(sListenSocket, sAcceptSocket, lpOutputBuffer
		, dwReceiveDataLength, dwLocalAddressLength
		, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
	
}



IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
						   
{	
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	mIoThreadCount = si.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mCompletionPort == NULL)
		return false;

	/// create TCP socket
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
		return false;

	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mCompletionPort, 0, 0);
	if (handle != mCompletionPort)
	{
		printf_s("[DEBUG] listen socket IOCP register error: %d\n", GetLastError());
		return false;
	}

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;

	//TODO : WSAIoctl�� �̿��Ͽ� AcceptEx, DisconnectEx �Լ� ��밡���ϵ��� �ϴ� �۾�..
	
	DWORD dwBytes = 0;

	int iResult = 0;
	// acceptEx ����� �κ�
	iResult = WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GGUIDAcceptEx, sizeof(GUID),
		&GAcceptEX, sizeof(LPFN_ACCEPTEX),
		&dwBytes, nullptr, nullptr);

	if (iResult == SOCKET_ERROR)
		return false;

	iResult = WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GGUIDDisconnectEx, sizeof(GUID),
		&GDisconnectEX, sizeof(LPFN_DISCONNECTEX),
		&dwBytes, nullptr, nullptr);

	if (iResult == SOCKET_ERROR)
		return false;

	/// make session pool
	GSessionManager->PrepareSessions();

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i+1), 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return false;
	}

	return true;
}


void IocpManager::StartAccept()
{
	/// listen

	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
	{
		printf_s("[DEBUG] listen error\n");
		return;
	}
		
	// ���⼭ ��� ���鼭 accept 
	while (GSessionManager->AcceptSessions())
	{
		Sleep(100);
		
	}

}


void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;
		
	LIoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = GIocpManager->GetComletionPort();

	while (true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ULONG_PTR completionKey = 0;

		int ret = GetQueuedCompletionStatus(hComletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

		ClientSession* theClient = context ? context->mSessionObject : nullptr ;
		
		if (ret == 0 || dwTransferred == 0)
		{
			int gle = GetLastError();

			//TODO: check time out first ... GQCS Ÿ�� �ƿ��� ���� ���?
			if (gle == WAIT_TIMEOUT)
				continue;

		
			if (context->mIoType == IO_RECV || context->mIoType == IO_SEND )
			{
				CRASH_ASSERT(nullptr != theClient);
			
				theClient->DisconnectRequest(DR_COMPLETION_ERROR);

				DeleteIoContext(context);

				continue;
			}
		}

		CRASH_ASSERT(nullptr != theClient);
	
		bool completionOk = false;
		switch (context->mIoType)
		{
		case IO_DISCONNECT:
			theClient->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
			completionOk = true;
			break;

		case IO_ACCEPT:
			theClient->AcceptCompletion();
			completionOk = true;
			break;

		case IO_RECV_ZERO:
			completionOk = PreReceiveCompletion(theClient, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;

		case IO_SEND:
			completionOk = SendCompletion(theClient, static_cast<OverlappedSendContext*>(context), dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(theClient, static_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			CRASH_ASSERT(false);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			theClient->DisconnectRequest(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

bool IocpManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	/// real receive...
	return client->PostRecv();
}

bool IocpManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);

	/// echo back
	return client->PostSend();
}

bool IocpManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->mWsaBuf.len != dwTransferred)
	{
		printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred) ;
		return false;
	}
	
	/// zero receive
	return client->PreRecv();
}


