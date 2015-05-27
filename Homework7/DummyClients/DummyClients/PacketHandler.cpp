#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "PacketInterface.h"
#include "DummyClientSession.h"


#include "MyPacket.pb.h"


//@{ Handler Helper

typedef void(*HandlerFunc)(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream);

static HandlerFunc HandlerTable[MAX_PKT_TYPE];

static void DefaultHandler(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)
{
	
	printf_s("Default Handler...PKT ID: %d\n", pktBase.mType);
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
	static void Handler_##PKT_TYPE(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)


//@}

void DummyClientSession::OnReceive(size_t len)
{
	
	/// 패킷 파싱하고 처리
	protobuf::io::ArrayInputStream arrayInputStream(mRecvBuffer.GetBufferStart(), mRecvBuffer.GetContiguiousBytes());
	protobuf::io::CodedInputStream codedInputStream(&arrayInputStream);

	PacketHeader packetheader;

	while (codedInputStream.ReadRaw(&packetheader, HEADER_SIZE))
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer(&payloadPos, &payloadSize);

		if ( payloadSize < packetheader.mSize ) ///< 패킷 본체 사이즈 체크
			break;

		if (packetheader.mType >= MAX_PKT_TYPE || packetheader.mType <= 0)
		{
			DisconnectRequest(DR_ACTIVE);
			break;
		}

		/// payload 읽기
		protobuf::io::ArrayInputStream payloadArrayStream(payloadPos, packetheader.mSize);
		protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);

		/// packet dispatch...
		HandlerTable[packetheader.mType](this, packetheader, payloadInputStream);
	
		/// 읽은 만큼 전진 및 버퍼에서 제거
		codedInputStream.Skip(packetheader.mSize); ///< readraw에서 헤더 크기만큼 미리 전진했기때문
		mRecvBuffer.Remove(HEADER_SIZE + packetheader.mSize);

	}
}

/////////////////////////////////////////////////////////////

using namespace MyPacket;

REGISTER_HANDLER(PKT_SC_LOGIN)
{
	LoginResult loginResult;
	if (false == loginResult.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	const Position& pos = loginResult.playerpos();

	session->mPlayer->SetId(loginResult.playerid());
	session->mPlayer->SetPosition(pos.x(), pos.y(), pos.z());
	std::wstring wName(loginResult.playername().begin(), loginResult.playername().end());
	session->mPlayer->SetName(wName.c_str());
	session->mPlayer->SetValid(true);

	printf_s("LOGIN SUCCESS: ID[%d], NAME[%s], POS[%f, %f, %f]\n", loginResult.playerid(), loginResult.playername().c_str(), pos.x(), pos.y(), pos.z());


	session->mPlayer->OnTick();
}

REGISTER_HANDLER(PKT_SC_CHAT)
{
	ChatResult chatResult;

	if (false == chatResult.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	printf_s("ID [%d] Get Message From Other Player : ", session->mPlayer->GetId());
	printf_s("[%s] : %s\n", chatResult.playername().c_str(), chatResult.playermessage().c_str());
}

REGISTER_HANDLER(PKT_SC_MOVE)
{
	MoveResult moveResult;

	if (false == moveResult.ParsePartialFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	Position movedPosition = moveResult.playerpos();
	session->mPlayer->SetPosition(movedPosition.x(), movedPosition.y(), movedPosition.z());

	printf_s("ID [%d] : Current Position is %2f, %2f, %2f \n", moveResult.playerid(), movedPosition.x(), movedPosition.y(), movedPosition.z());

}