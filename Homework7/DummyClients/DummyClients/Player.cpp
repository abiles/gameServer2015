#include "stdafx.h"
#include "Player.h"
#include "Timer.h"
#include "DummyClientSession.h"
#include "ThreadLocal.h"

Player::Player(DummyClientSession* session) : mSession(session)
{
	memset(mPlayerName, 0, sizeof(mPlayerName));
	memset(mComment, 0, sizeof(mComment));

	mPlayerId = -1;
	mIsValid = false;
	mPosX = mPosY = mPosZ = 0;
}


Player::~Player()
{
}

void Player::SetPosition(float x, float y, float z)
{
	mPosX = x;
	mPosY = y;
	mPosZ = z;
}

void Player::SetId(int id)
{
	mPlayerId = id;
}

void Player::SetValid(bool valid)
{
	mIsValid = valid;
}

void Player::SetName(const wchar_t* name)
{
	wcscpy_s(mPlayerName, name);
}

void Player::SetComment(const wchar_t* comment)
{
	wcscpy_s(mComment, comment);
}

void Player::Set_x(float x)
{
	mPosX = x;
}

void Player::Set_y(float y)
{
	mPosY = y;
}

void Player::Set_z(float z)
{
	mPosZ = z;
}

void Player::OnTick()
{
	if (!IsVaild())
		return;
	
	
	if (rand() % 2)
	{
		mSession->Move();
	}
	else
	{
		int choose = rand() % 3;

		if (choose == 0)
		{
			mSession->Chat(L"I am still hungry");
		}
		else if (choose == 1)
		{
			mSession->Chat(L"I do not like curry");
		}
		else
		{
			mSession->Chat(L"Frostmourne Hungers");
		}
	}


	DoSyncAfter(HEART_BEAT, GetSharedFromThis<Player>(), &Player::OnTick);
}
