#pragma once

#include "PacketInterface.h"
#include "Session.h"
#include "Player.h"

class DummyClientSession : public Session, public ObjectPool<DummyClientSession>
{
public:
	DummyClientSession();
	virtual ~DummyClientSession();

	bool PrepareSession();

	bool ConnectRequest();
	void ConnectCompletion();

	bool SendRequest(short packetType, const protobuf::MessageLite& payload);

	virtual void OnReceive(size_t len);
	virtual void OnRelease();

	void Login();
	void Move();
	void Chat(wchar_t* message);

public:
	std::shared_ptr<Player> mPlayer;

private:
	
	SOCKADDR_IN mConnectAddr;
} ;



