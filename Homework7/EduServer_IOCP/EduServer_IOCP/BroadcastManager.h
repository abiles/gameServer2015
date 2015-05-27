#pragma once
#include "XTL.h"
#include "FastSpinlock.h"
#include "MyPacket.pb.h"

using namespace MyPacket;

struct PacketHeader;
class ClientSession;

class BroadcastManager : public std::enable_shared_from_this<BroadcastManager>
{
public:
	BroadcastManager();
	virtual ~BroadcastManager() {}

	void RegisterClient(ClientSession* client);
	void UnregisterClient(ClientSession* client);

	void BroadcastPacket(google::protobuf::MessageLite* pkt);

private:
	FastSpinlock mLock;

	xset<ClientSession*>::type mConnectedClientSet;
	int mCurrentConnectedCount;

};

extern std::shared_ptr<BroadcastManager> GBroadcastManager;