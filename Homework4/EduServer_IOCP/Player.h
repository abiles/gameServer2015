#pragma once

#include "SyncExecutable.h"
class ClientSession;


class Player : public SyncExecutable
{
public:
	Player(ClientSession* session);
	virtual ~Player();

	int GetPlayerId() { return mPlayerId; }
	bool IsAlive() { return mIsAlive;  }
	void Start(int heartbeat);

	void OnTick(); ///< �α����� 1�ʸ��� �Ҹ��� ���

	void PlayerReset();

	/// �÷��̾�� ������ �ɾ��ִ� �Լ���� ġ��.
	void AddBuff(int fromPlayerId, int buffId, int duration);

	/// �ֱ������� ���� �ð� ������Ʈ�ϴ� �Լ�
	void DecayTickBuff();

private:

	int		mPlayerId;
	int		mHeartBeat;
	bool	mIsAlive;

	/// ���� ����Ʈ�� lock���� GCE�� �غ���
	std::map<int, int> mBuffList; ///< (id, time)

	ClientSession* const mSession;
	friend class ClientSession;
};