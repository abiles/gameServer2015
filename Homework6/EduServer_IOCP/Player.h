#pragma once
#include "ContentsConfig.h"

class ClientSession;

class Player
{
public:
	Player(ClientSession* session);
	~Player();

	bool IsLoaded() { return mPlayerId > 0; }
	
	void RequestLoad(int pid);
	void ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment);

	void RequestUpdatePosition(float x, float y, float z);
	void ResponseUpdatePosition(float x, float y, float z);

	void RequestUpdateComment(const wchar_t* comment);
	void ResponseUpdateComment(const wchar_t* comment);

	void RequestUpdateValidation(bool isValid);
	void ResponseUpdateValidation(bool isValid);

	int  GetPlayerID() const { return mPlayerId; }

private:

	void PlayerReset();

	//TODO: �Ʒ� 2�� �׽�Ʈ �Լ��� ���������� �����ϴ��� Ȯ���ϱ�
	void TestCreatePlayerData(const wchar_t* newName);
	void TestDeletePlayerData(int playerId);

private:

	FastSpinlock mPlayerLock;
	int		mPlayerId;
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	bool	mIsValid;
	wchar_t	mPlayerName[MAX_NAME_LEN];
	wchar_t	mComment[MAX_COMMENT_LEN];

	ClientSession* const mSession;
	friend class ClientSession;
};