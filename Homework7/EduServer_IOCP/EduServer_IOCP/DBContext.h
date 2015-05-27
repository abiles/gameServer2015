#pragma once

#include "ObjectPool.h"



class ClientSession;

/// ��� DB���� �۾��� �Ʒ��� �߻� Ŭ������ ��ӹ޾� ����ؾ� ��
struct DatabaseJobContext
{
	DatabaseJobContext(ClientSession* owner);
	virtual ~DatabaseJobContext();

	void OnResult();

	bool SQLExecute();

	virtual bool OnSQLExecute() = 0;	///< DBǮ���� ����Ǵ� SQL �۾�
	virtual void OnSuccess() {}		///< SQL �۾� ������ ����Ǯ���� ���� ����Ǿ�� �ϴ� ����
	virtual void OnFail() {}		///< SQL �۾� ���н� ����Ǯ���� ���� ����Ǿ�� �ϴ� ����


	ClientSession* mSessionObject;
	bool mSuccess;
};

