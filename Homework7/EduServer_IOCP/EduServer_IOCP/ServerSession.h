#pragma once

#include "MemoryPool.h"
#include "Session.h"

/// �ٸ� �������� ������ ���� ����
class ServerSession : public Session, public PooledAllocatable
{
public:
	ServerSession(const char* serverAddr, unsigned short port);
	virtual ~ServerSession();

	bool ConnectRequest();
	void ConnectCompletion();


private:
	const char*		mServerAddr;
	unsigned short	mPort;

};