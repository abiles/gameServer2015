#include "stdafx.h"
#include "Log.h"
#include "PacketType.h"
#include "Player.h"
#include "ClientSession.h"
#include "BroadcastManager.h"
#include "MyPacket.pb.h"

using namespace MyPacket;

//@{ Handler Helper

/// �ϴ� ���� ũ�� ��Ŷ�� ���

typedef void(*HandlerFunc)(ClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream);

static HandlerFunc HandlerTable[MAX_PKT_TYPE];

static void DefaultHandler(ClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)
{
	LoggerUtil::EventLog("invalid packet handler", session->mPlayer->GetPlayerId());
	session->DisconnectRequest(DR_ACTIVE);
}

struct InitializeHandlers
{
	InitializeHandlers()
	{
		for (int i = 0; i < MAX_PKT_TYPE; ++i)
			HandlerTable[i] = DefaultHandler;
	}
} _init_handlers_;

struct RegisterHandler
{
	RegisterHandler(int pktType, HandlerFunc handler)
	{
		HandlerTable[pktType] = handler;
	}
};

#define REGISTER_HANDLER(PKT_TYPE)	\
	static void Handler_##PKT_TYPE(ClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)

//@}


void ClientSession::OnRead(size_t len)
{
	protobuf::io::ArrayInputStream arrayInputStream(mRecvBuffer.GetBufferStart(), mRecvBuffer.GetContiguiousBytes());
	protobuf::io::CodedInputStream codedInputStream(&arrayInputStream);

	PacketHeader packetheader;

	/// ��Ŷ �Ľ��ϰ� ó��
	while (codedInputStream.ReadRaw(&packetheader, HEADER_SIZE))
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer(&payloadPos, &payloadSize);
		
		/// ��Ŷ ��� ũ�� ��ŭ �о�ͺ���
		if (false == mRecvBuffer.Peek((char*)&packetheader, sizeof(PacketHeader)))
			return;

		/// ��Ŷ �ϼ��� �Ǵ°�? 
		if (mRecvBuffer.GetStoredSize() < packetheader.mSize)
			return;


		if (packetheader.mType >= MAX_PKT_TYPE || packetheader.mType <= 0)
		{
			LoggerUtil::EventLog("packet type error", mPlayer->GetPlayerId());
			DisconnectRequest(DR_ACTIVE);
			return;
		}

		/// payload �б�
		protobuf::io::ArrayInputStream payloadArrayStream(payloadPos, packetheader.mSize);
		protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);

		/// packet dispatch...
		HandlerTable[packetheader.mType](this, packetheader, payloadInputStream);

		/// ���� ��ŭ ���� �� ���ۿ��� ����
		codedInputStream.Skip(packetheader.mSize); 
		mRecvBuffer.Remove(HEADER_SIZE + packetheader.mSize);
	}
}

/////////////////////////////////////////////////////////
REGISTER_HANDLER(PKT_CS_LOGIN)
{
	LoginRequest inPacket;
	if (false == inPacket.ParseFromCodedStream(&payloadStream))
	{
		LoggerUtil::EventLog("packet parsing error", inPacket.playerid());
		return;
	}
	
	/// �׽�Ʈ�� ���� 10ms�Ŀ� �ε��ϵ��� ����
	DoSyncAfter(10, session->mPlayer, &Player::RequestLoad, inPacket.playerid());

}

REGISTER_HANDLER(PKT_CS_MOVE)
{
	MoveRequest inPacket;
	if (false == inPacket.ParseFromCodedStream(&payloadStream))
	{
		LoggerUtil::EventLog("packet parsing error", inPacket.playerid());
		return;
	}

	if (inPacket.playerid() != session->mPlayer->GetPlayerId())
	{
		LoggerUtil::EventLog("PKT_CS_MOVE: invalid player ID", session->mPlayer->GetPlayerId());
		return;
	}

	Position requsetPos = inPacket.playerpos();
	/// ������ ���� �׽�Ʈ�� ���� DB�� ������Ʈ�ϰ� �뺸�ϵ��� ����.
	session->mPlayer->DoSync(&Player::RequestUpdatePosition, requsetPos.x(), requsetPos.y(), requsetPos.z());
}

REGISTER_HANDLER(PKT_CS_CHAT)
{
	ChatRequest inPacket;
	if (false == inPacket.ParseFromCodedStream(&payloadStream))
	{
		LoggerUtil::EventLog("packet parsing error", inPacket.playerid());
		return;
	}

	if (inPacket.playerid() != session->mPlayer->GetPlayerId())
	{
		LoggerUtil::EventLog("PKT_CS_CHAT: invalid player ID", session->mPlayer->GetPlayerId());
		return;
	}

	/// chatting�� ��� ���⼭ �ٷ� ���
	ChatResult outPacket;
	
	std::wstring playerName = session->mPlayer->GetPlayerName();
	std::string name;
	std::string message = inPacket.playermessage();
	
	name.assign(playerName.begin(), playerName.end());

	outPacket.set_playername(name);
	outPacket.set_playermessage(message);

	GBroadcastManager->BroadcastPacket(&outPacket);
	
}

