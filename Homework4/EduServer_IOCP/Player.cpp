#include "stdafx.h"
#include "Timer.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerManager.h"
#include "PlayerWideEvent.h"
#include "GrandCentralExecuter.h"

Player::Player(ClientSession* session) : mSession(session)
{
	PlayerReset();
}

Player::~Player()
{
	CRASH_ASSERT(false);
}

void Player::PlayerReset()
{
	/// �÷��̾� �ʿ��� ����
	GPlayerManager->UnregisterPlayer(mPlayerId);

	mPlayerId = -1;
	mHeartBeat = -1;
	mIsAlive = false;
}

void Player::Start(int heartbeat)
{
	mIsAlive = true;
	mHeartBeat = heartbeat;
	
	/// ID �߱� �� �÷��׾� �ʿ� ���
	mPlayerId = GPlayerManager->RegisterPlayer(GetSharedFromThis<Player>());

	/// ���� �Ҿ�ֱ� ����
	OnTick();

}

void Player::OnTick()
{
	if (!IsAlive())
		return;

	
	/// �������� �̺�Ʈ�� �߻����Ѻ��� (��: �ٸ� ��� �÷��̾�� ���� �ֱ�)
	if (rand() % 100 == 0) ///< 1% Ȯ��
	{
		int buffId = mPlayerId * 100;
		int duration = (rand() % 3 + 2) * 1000;
	
		//GCE ��. (lock-order ������, ���������� ���� ���� �ʿ��� ��)
		auto playerEvent = std::make_shared<AllPlayerBuffEvent>(buffId, duration);
		GCEDispatch(playerEvent, &AllPlayerBuffEvent::DoBuffToAllPlayers, mPlayerId);
	}


	//TODO: AllPlayerBuffDecay::CheckBuffTimeout�� GrandCentralExecuter�� ���� ����

	
	
	if (mHeartBeat > 0)
		DoSyncAfter(mHeartBeat, GetSharedFromThis<Player>(), &Player::OnTick);
		
}

void Player::AddBuff(int fromPlayerId, int buffId, int duration)
{
	printf_s("I am Buffed [%d]! from Player [%d]\n", buffId, fromPlayerId);

	/// �÷��̾��� ���� ����Ʈ�� �߰�
	mBuffList.insert(std::make_pair(buffId, duration));
}

void Player::DecayTickBuff()
{
	/// ������ ���� �ð��� �ֱ������� �����ϰ�, �ð��� ����Ǹ� �����ϱ�
	for (auto it = mBuffList.begin(); it != mBuffList.end();)
	{
		it->second -= mHeartBeat;

		if (it->second <= 0)
		{
			printf_s("Player [%d] BUFF [%d] expired\n", mPlayerId, it->first);
			mBuffList.erase(it++);
		}
		else
		{
			++it;
		}
	}
}