#pragma once

#include "FastSpinlock.h"
#include "XTL.h"

class Player;
typedef xvector<std::shared_ptr<Player>>::type PlayerList;

class PlayerManager
{
public:
	PlayerManager();

	/// �÷��̾ ����ϰ� ID�� �߱�
	int RegisterPlayer(std::shared_ptr<Player> player);

	/// ID�� �ش��ϴ� �÷��̾� ����
	void UnregisterPlayer(int playerId);

	int GetCurrentPlayers(PlayerList& outList);
	
private:
	FastSpinlock mLock;
	int mCurrentIssueId;
	xmap<int, std::shared_ptr<Player>>::type mPlayerMap;
};

extern PlayerManager* GPlayerManager;
