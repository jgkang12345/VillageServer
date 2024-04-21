#include "pch.h"
#include "VillageServerConnection.h"
#include "ConnectionContext.h"
#include "PacketHandler.h"
#include "MapManager.h"
#include "Player.h"
VillageServerConnection::VillageServerConnection(const SOCKET& socket, const SOCKADDR_IN& sockAddrIn)
	:Connection(socket, sockAddrIn)
{
}

VillageServerConnection::VillageServerConnection() : Connection()
{
}

VillageServerConnection::~VillageServerConnection()
{

}

void VillageServerConnection::OnRecv(Connection* connection, byte* dataPtr, int32 dataLen)
{
	PacketHandler::HandlePacket(static_cast<VillageServerConnection*>(connection), dataPtr, dataLen);
}

void VillageServerConnection::OnDisconnect()
{
	if (InterlockedExchange64(&_disConnect, 1) == 1)
	{
		return;
	}


	{
		int64 desired = 1;
		int64 expected = 0;
		if (InterlockedCompareExchange64(&_deletePlayer, desired, expected) == 1)
			return;
	}

	// TODO 현재 정보 저장
	if (_player)
	{
		DBConnection* playerCon = PlayerDBConnectionPool::GetInstance()->Pop();
		Vector3 playerPos = { 0,0,0 };
		// 로그아웃 시간 DB에 저장
		{
			// hp, exp, level, damage, speed
			int32 hp = _player->GetHp();
			int32 exp = _player->GetExp();
			int32 level = _player->GetLevel();
			int32 damage = _player->GetDamage();
			float speed = _player->GetSpeed();
			int32 userSq = _player->GetPlayerSQ();
			SQLLEN len;
			SQLPrepare(playerCon->GetHSTMT(), (SQLWCHAR*)L"UPDATE  player.d_player SET hp = ? , exp = ? , level = ? , damage = ? , speed = ? WHERE player_sq = ? ; ", SQL_NTS);
			SQLBindParameter(playerCon->GetHSTMT(), 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&hp, 0, NULL);
			SQLBindParameter(playerCon->GetHSTMT(), 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&exp, 0, NULL);
			SQLBindParameter(playerCon->GetHSTMT(), 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&level, 0, NULL);
			SQLBindParameter(playerCon->GetHSTMT(), 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&damage, 0, NULL);
			SQLBindParameter(playerCon->GetHSTMT(), 5, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_FLOAT, 0, 0, (SQLFLOAT*)&speed, 0, NULL);
			SQLBindParameter(playerCon->GetHSTMT(), 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&userSq, 0, NULL);
			SQLExecute(playerCon->GetHSTMT());
			SQLFetch(playerCon->GetHSTMT());
			SQLCloseCursor(playerCon->GetHSTMT());
		}
	}

	if (_player)
	{
		MapManager::GetInstance()->ReSet(_player);
	}
	ConnectionContext::GetInstance()->RemoveConnection(_connectionId);
	closesocket(_socket);
	delete this;
}

void VillageServerConnection::OnConnect()
{
}
