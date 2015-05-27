#pragma once
#include "SyncExecutable.h"

#define MAX_NAME_LEN	32
#define MAX_COMMENT_LEN	256
#define HEART_BEAT 1000


class DummyClientSession;

class Player : public SyncExecutable
{
public:
	Player(DummyClientSession* session);
	~Player();


	void		SetId(int id);
	int			GetId(){ return mPlayerId; };

	void		SetValid(bool valid);
	bool		IsVaild() { return mIsValid; };

	void		SetName(const wchar_t* name);
	wchar_t*	GetName() { return mPlayerName; };

	void		SetComment(const wchar_t* comment);
	wchar_t*    GetComment() { return mComment; };

	void		SetPosition(float x, float y, float z);
	void		Set_x(float x);
	void		Set_y(float y);
	void		Set_z(float z);

	float		x(){ return mPosX; };
	float		y(){ return mPosY; };
	float		z(){ return mPosZ; };

	void		OnTick();

private:
	int			mPlayerId;
	float		mPosX;
	float		mPosY;
	float		mPosZ;
	bool		mIsValid;
	wchar_t		mPlayerName[MAX_NAME_LEN];
	wchar_t     mComment[MAX_COMMENT_LEN];
	
	DummyClientSession* const mSession;
	friend class DummyClientSession;

};

