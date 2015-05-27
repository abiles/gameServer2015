#include "stdafx.h"
#include "Log.h"
#include "PacketType.h"
#include "Player.h"
#include "ClientSession.h"
#include "BroadcastManager.h"
#include "MyPacket.pb.h"

using namespace MyPacket;

//@{ Handler Helper

/// 일단 고정 크기 패킷만 취급

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

	/// 패킷 파싱하고 처리
	while (codedInputStream.ReadRaw(&packetheader, HEADER_SIZE))
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer(&payloadPos, &payloadSize);
		
		/// 패킷 헤더 크기 만큼 읽어와보기
		if (false == mRecvBuffer.Peek((char*)&packetheader, sizeof(PacketHeader)))
			return;

		/// 패킷 완성이 되는가? 
		if (mRecvBuffer.GetStoredSize() < packetheader.mSize)
			return;


		if (packetheader.mType >= MAX_PKT_TYPE || packetheader.mType <= 0)
		{
			LoggerUtil::EventLog("packet type error", mPlayer->GetPlayerId());
			DisconnectRequest(DR_ACTIVE);
			return;
		}

		/// payload 읽기
		protobuf::io::ArrayInputStream payloadArrayStream(payloadPos, packetheader.mSize);
		protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);

		/// packet dispatch...
		HandlerTable[packetheader.mType](this, packetheader, payloadInputStream);

		/// 읽은 만큼 전진 및 버퍼에서 제거
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
	
	/// 테스트를 위해 10ms후에 로딩하도록 ㄱㄱ
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
	/// 지금은 성능 테스트를 위해 DB에 업데이트하고 통보하도록 하자.
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

	/// chatting의 경우 여기서 바로 방송
	ChatResult outPacket;
	
	std::wstring playerName = session->mPlayer->GetPlayerName();
	std::string name;
	std::string message = inPacket.playermessage();
	
	name.assign(playerName.begin(), playerName.end());

	outPacket.set_playername(name);
	outPacket.set_playermessage(message);

	GBroadcastManager->BroadcastPacket(&outPacket);
	
}

