#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"


bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//TODO: 이 영역 lock으로 보호 할 것
	FastSpinlockGuard lockGuard(mLock);

	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);
		
	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}

	//TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것
	HANDLE hPort = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
	
	if (hPort != GIocpManager->GetComletionPort())
	{	
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();


	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: 이 영역 lock으로 할 것
	FastSpinlockGuard lockGuard(mLock);


	if ( !IsConnected() )
		return ;
	
	LINGER lingerOption ;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) )
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	GSessionManager->DecreaseConnectionCount();

	closesocket(mSocket) ;

	mConnected = false ;


}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//TODO: WSARecv 사용하여 구현할 것
	DWORD flag = 0;
	recvContext->mWsaBuf.buf = recvContext->mBuffer;
	recvContext->mWsaBuf.len = BUFSIZE;

	DWORD recvBytes = 0;
	int ret = 0;

	if(ret = WSARecv(mSocket
		  , &(recvContext->mWsaBuf)
		  , 1, &recvBytes, &flag
		  , &(recvContext->mOverlapped)
		  , NULL));

	if (SOCKET_ERROR == ret)
	{
		if (WSA_IO_PENDING != GetLastError())
		{
			return false;
		}
	}


	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);

	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	sendContext->mWsaBuf.len = BUFSIZE;

	//TODO: WSASend 사용하여 구현할 것
	DWORD flag = 0;
	DWORD sendBytes = 0;
	int ret = 0;

	if(ret = WSASend(mSocket
		, &(sendContext->mWsaBuf)
		, 1, &sendBytes, flag
		, &(sendContext->mOverlapped)
		, NULL));

	if (SOCKET_ERROR == ret)
	{
		if (WSA_IO_PENDING != GetLastError())
		{
			return false;
		}
	}

	return true;
}