#include "stdafx.h"
#include "Timer.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerDBContext.h"
#include "DBManager.h"
#include "GrandCentralExecuter.h"

#include "PacketType.h"
#include "BroadcastManager.h"
#include "MyPacket.pb.h"

using namespace MyPacket;

Player::Player(ClientSession* session) : mSession(session)
{
	PlayerReset();
}

Player::~Player()
{
}

void Player::PlayerReset()
{
	memset(mPlayerName, 0, sizeof(mPlayerName));
	memset(mComment, 0, sizeof(mComment));
	mPlayerId = -1;
	mIsValid = false;
	mPosX = mPosY = mPosZ = 0;
}

void Player::RequestLoad(int pid)
{
 	LoadPlayerDataContext* context = new LoadPlayerDataContext(mSession, pid);
 	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment)
{
	mPlayerId = pid;
	mPosX = x;
	mPosY = y;
	mPosZ = z;
	mIsValid = valid;

	wcscpy_s(mPlayerName, name);
	wcscpy_s(mComment, comment);

	//TODO: 아래는 나중에 로그로...
	wprintf_s(L"PID[%d], X[%f] Y[%f] Z[%f] NAME[%s] COMMENT[%s]\n", mPlayerId, mPosX, mPosY, mPosZ, name, mComment);
	
	DoSyncAfter(HEART_BEAT, GetSharedFromThis<Player>(), &Player::OnTick);

	LoginResult outPacket;
	
	//ID
	outPacket.set_playerid(mPlayerId);
	
	//Pos
	Position* playerPos = new Position();
	playerPos->set_x(mPosX);
	playerPos->set_y(mPosY);
	playerPos->set_z(mPosZ);
	outPacket.set_allocated_playerpos(playerPos);

	//name
	std::wstring playerName(mPlayerName);
	std::string sName;
	playerName.assign(sName.begin(), sName.end());
	outPacket.set_playername(sName);

	mSession->SendRequest(PKT_SC_LOGIN, outPacket);
	
}

void Player::RequestUpdatePosition(float x, float y, float z)
{
	UpdatePlayerPositionContext* context = new UpdatePlayerPositionContext(mSession, mPlayerId);
	context->mPosX = x;
	context->mPosY = y;
	context->mPosZ = z;
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdatePosition(float x, float y, float z)
{
	mPosX = x;
	mPosY = y;
	mPosZ = z;

	MoveResult outPacket;

	outPacket.set_playerid(mPlayerId);

	Position* playerPos = new Position();
	playerPos->set_x(mPosX);
	playerPos->set_y(mPosY);
	playerPos->set_z(mPosZ);
	outPacket.set_allocated_playerpos(playerPos);


	mSession->SendRequest(PKT_SC_MOVE, outPacket);
}

void Player::RequestUpdateComment(const wchar_t* comment)
{
	UpdatePlayerCommentContext* context = new UpdatePlayerCommentContext(mSession, mPlayerId);
	context->SetNewComment(comment);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateComment(const wchar_t* comment)
{
	wcscpy_s(mComment, comment);
}

void Player::RequestUpdateValidation(bool isValid)
{
	UpdatePlayerValidContext* context = new UpdatePlayerValidContext(mSession, mPlayerId);
	context->mIsValid = isValid;
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateValidation(bool isValid)
{
	mIsValid = isValid;
}


void Player::TestCreatePlayerData(const wchar_t* newName)
{
	CreatePlayerDataContext* context = new CreatePlayerDataContext(mSession);
	context->SetNewName(newName);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::TestDeletePlayerData(int playerId)
{
	DeletePlayerDataContext* context = new DeletePlayerDataContext(mSession, playerId);
	GDatabaseManager->PostDatabsaseRequest(context);
}


void Player::OnTick()
{
	if (!IsValid())
		return;



// lock order 테스트
	//static FastSpinlock testlock(LO_BUSINESS_CLASS);
	//testlock.EnterReadLock();
	//printf("tick: lockflag %d, use count %d\n", testlock.GetLockFlag(), GetSharedFromThis<Player>().use_count());
	//testlock.LeaveReadLock();

	DoSyncAfter(HEART_BEAT, GetSharedFromThis<Player>(), &Player::OnTick);
}



// 	class PlayerJob : public std::enable_shared_from_this<PlayerJob>
// 	{
// 	public:
// 		PlayerJob(int pid) : mPID(pid)
// 		{
// 			printf("ctor %d\n ", mPID);
// 		}
// 		virtual ~PlayerJob()
// 		{
// 			printf("dtor %d\n ", mPID);
// 		}
// 
// 		void TestFunc(int a)
// 		{
// 			printf("     Hello %d %d\n", mPID, a);
// 		}
// 
// 	private:
// 		int mPID;
// 	};
// 
// 	auto p = std::make_shared<PlayerJob>(mPlayerId);
// 
// 	GCEDispatch(p, &PlayerJob::TestFunc, mPlayerId+1);