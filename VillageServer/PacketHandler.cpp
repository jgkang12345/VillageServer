#include "pch.h"
#include "PacketHandler.h"
#include "VillageServerConnection.h"
#include "ConnectionContext.h"
#include "MapManager.h"
#include "Player.h"
#include "ThreadSafeSharedPtr.h"
#include "MonsterManager.h"
void PacketHandler::HandlePacket(VillageServerConnection* connection, BYTE* packet, int32 packetSize)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet);
	byte* dataPtr = packet + sizeof(PacketHeader);
	int32 dataSize = packetSize - sizeof(PacketHeader);

	switch (header->_type)
	{
	case C2S_PLAYERINIT:
		HandlePacket_C2S_PLAYERINIT(connection, packet, dataSize);
		break;
	case C2S_PLAYERSYNC:
		HandlePacket_C2S_PLAYERSYNC(connection, dataPtr, dataSize);
		break;
	case C2S_MAPSYNC:
		HandlePacket_C2S_MAPSYNC(connection, dataPtr, dataSize);
		break;
	case C2S_PLAYERCHAT:
		HandlePacket_C2S_PLAYERCHAT(connection, dataPtr, dataSize);
		break;
	case C2S_LATENCY: // 모니터 레이턴시
		HandlePacket_C2S_LATENCY(connection, dataPtr, dataSize);
		break;
	case S2C_HEARTBIT:
		HandlePacket_S2C_HEARTBIT(connection, dataPtr, dataSize);
		break;

	case PacketProtocol::C2S_PLAYERATTACK:
		HandlePacket_C2S_PLAYERATTACK(connection, dataPtr, dataSize);
		break;

	case PacketProtocol::C2S_MONSTERATTACKED:
		HandlePacket_C2S_MONSTERATTACKED(connection, dataPtr, dataSize);
		break;

	case PacketProtocol::C2S_PLAYERESPAWN:
		HandlePacket_C2S_PLAYERESPAWN(connection, dataPtr, dataSize);
		break;
	}
}

void PacketHandler::HandlePacket_C2S_PLAYERINIT(VillageServerConnection* connection, BYTE* dataPtr, int32 dataSize)
{
	PLAYERINIT* playerInit = reinterpret_cast<PLAYERINIT*>(dataPtr);

	connection->SetConnectionId(playerInit->userSQ);
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();

	WCHAR playerName[256] = {};
	int32 level;
	int32 hp;
	int32 mp;
	Vector3 pos;
	int32 damage;
	float speed;
	int32 defense;
	int32 playerType;
	int32 exp;

	{
		SQLPrepare(con->GetHSTMT(), (SQLWCHAR*)L"select top 1 player_name,level,hp,mp,x,y,z,damage,speed,defense,player_type,exp from player.d_player where PLAYER_SQ = ?;", SQL_NTS);
		SQLBindParameter(con->GetHSTMT(), 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLINTEGER*)&playerInit->playerSQ, 0, NULL);
		SQLLEN len = 0;

		SQLBindCol(con->GetHSTMT(), 1, SQL_WCHAR, (SQLWCHAR*)&playerName, sizeof(playerName), &len);
		SQLBindCol(con->GetHSTMT(), 2, SQL_INTEGER, &level, sizeof(level), &len);
		SQLBindCol(con->GetHSTMT(), 3, SQL_INTEGER, &hp, sizeof(hp), &len);
		SQLBindCol(con->GetHSTMT(), 4, SQL_INTEGER, &mp, sizeof(mp), &len);
		SQLBindCol(con->GetHSTMT(), 5, SQL_C_FLOAT, &pos.x, sizeof(pos.x), &len);
		SQLBindCol(con->GetHSTMT(), 6, SQL_C_FLOAT, &pos.y, sizeof(pos.y), &len);
		SQLBindCol(con->GetHSTMT(), 7, SQL_C_FLOAT, &pos.z, sizeof(pos.z), &len);
		SQLBindCol(con->GetHSTMT(), 8, SQL_INTEGER, &damage, sizeof(damage), &len);
		SQLBindCol(con->GetHSTMT(), 9, SQL_C_FLOAT, &speed, sizeof(speed), &len);
		SQLBindCol(con->GetHSTMT(), 10, SQL_INTEGER, &defense, sizeof(defense), &len);
		SQLBindCol(con->GetHSTMT(), 11, SQL_INTEGER, &playerType, sizeof(playerType), &len);
		SQLBindCol(con->GetHSTMT(), 12, SQL_INTEGER, &exp, sizeof(exp), &len);
		SQLExecute(con->GetHSTMT());
		SQLFetch(con->GetHSTMT());
		SQLCloseCursor(con->GetHSTMT());
	}

	if (pos.Zero())
	{
		// 처음 접속인 경우 어디에 태어나게 할건가?
		pos = { 45,0,50 };
	}

	Player* newPlayer = new Player(
		  static_cast<Connection*>(connection)	
		, pos
		, playerName
		, level
		, hp
		, mp
		, damage
		, speed
		, defense
		, playerType
		, playerInit->playerSQ
		, exp);

	ConnectionContext::GetInstance()->AddConnetion(playerInit->userSQ, connection);

	{
		int32 sessionId = newPlayer->GetConnection()->GetConnectionId();
		int8 playerState = (int8)newPlayer->GetState();
		int8 playerDir = (int8)newPlayer->GetDir();
		int8 playerMouseDir = (int8)newPlayer->GetMouseDir();
		Vector3 playerPos = newPlayer->GetPos();
		Quaternion playerQuaternion = newPlayer->GetCameraLocalRotation();
		float hp = newPlayer->GetHp();
		float mp = newPlayer->GetMp();
		int8 level = newPlayer->GetLevel();
		float spped = newPlayer->GetSpeed();
		float damage = newPlayer->GetDamage();
		WCHAR* userName = newPlayer->GetPlayerName();
		int8 userNameSize = (int8)(wcslen(userName) * sizeof(WCHAR));
		int8 playerType = (int8)newPlayer->GetPlayerType();
		int32 exp = newPlayer->GetExp();

		S2C_PLAYERINIT_PACKET* sendPacket = new S2C_PLAYERINIT_PACKET();
		sendPacket->sessionId = sessionId;
		sendPacket->playerState = playerState;
		sendPacket->playerDir = playerDir;
		sendPacket->playerMouseDir = playerMouseDir;
		sendPacket->playerPos = playerPos;
		sendPacket->playerQuaternion = playerQuaternion;
		sendPacket->hp = hp;
		sendPacket->mp = mp;
		sendPacket->level = level;
		sendPacket->speed = spped;
		sendPacket->damage = damage;
		wcscpy_s(sendPacket->playerName, userName);
		sendPacket->playerType = playerType;
		sendPacket->exp = exp;
		ThreadSafeSharedPtr safeSendPacket = ThreadSafeSharedPtr(sendPacket, false);
		connection->Send(safeSendPacket);
	}
	connection->SetPlayer(newPlayer);
	MapManager::GetInstance()->Set(newPlayer);
	PlayerDBConnectionPool::GetInstance()->Push(con);
}

void PacketHandler::HandlePacket_C2S_PLAYERSYNC(VillageServerConnection* connection, BYTE* packet, int32 packetSize)
{
	Player* player = connection->GetPlayer();
	BinaryReader br(packet);

	int32 playerId;
	State state;
	Dir dir;
	Dir mouseDir;
	Vector3 vector3;
	Quaternion quaternion;
	Vector3 target;
	MoveType moveType;
	Vector3 angle;
	br.Read(playerId);
	br.Read(state);
	br.Read(dir);
	br.Read(mouseDir);
	br.Read(vector3);
	br.Read(quaternion);
	br.Read(target);
	br.Read(angle);
	br.Read(moveType);

	player->PlayerSync(vector3, state, dir, mouseDir, quaternion, target, moveType, angle);

	// BYTE sendBuffer[100];
	byte* sendBuffer = new byte[500]; 

	BinaryWriter bw(sendBuffer);
	PacketHeader* pktHeader = bw.WriteReserve<PacketHeader>();

	bw.Write(connection->GetConnectionId());		// 4
	bw.Write((int8)player->GetState());				// 1
	bw.Write((int8)player->GetDir());				// 1
	bw.Write((int8)player->GetMouseDir());			// 1
	bw.Write(player->GetPos());						// 12
	bw.Write(player->GetCameraLocalRotation());		// 16
	bw.Write(target);								// 12
	bw.Write(angle);								// 12
	bw.Write((int8)moveType);						// 1

	pktHeader->_type = PacketProtocol::S2C_PLAYERSYNC;
	pktHeader->_pktSize = bw.GetWriterSize();

	ThreadSafeSharedPtr safeSendBuffer = ThreadSafeSharedPtr(reinterpret_cast<PACKET_HEADER*>(sendBuffer), false);
	MapManager::GetInstance()->BroadCast(connection->GetPlayer(), safeSendBuffer);
}

void PacketHandler::HandlePacket_C2S_MAPSYNC(VillageServerConnection* connection, BYTE* packet, int32 packetSize)
{
	Player* player = connection->GetPlayer();
	Vector3 prevPos = player->GetPrevPos();
	BinaryReader br(packet);

	int32 playerId;
	State state;
	Dir dir;
	Dir mouseDir;
	Vector3 vector3;
	Quaternion quaternion;
	Vector3 target;
	MoveType moveType;
	Vector3 angle;

	br.Read(playerId);
	br.Read(state);
	br.Read(dir);
	br.Read(mouseDir);
	br.Read(vector3);
	br.Read(quaternion);
	br.Read(target);
	br.Read(moveType);
	br.Read(angle);

	player->PlayerSync(vector3, state, dir, mouseDir, quaternion, target, moveType, angle);
	MapManager::GetInstance()->MapSync(connection->GetPlayer());
	player->SetPrevPos(vector3);
}

void PacketHandler::HandlePacket_C2S_LATENCY(VillageServerConnection* session, BYTE* packet, int32 packetSize)
{
	BinaryReader br(packet);
	int32 lastTick;
	br.Read(lastTick);

	byte* sendBuffer = new byte[100];
	BinaryWriter bw(sendBuffer);
	PacketHeader* pktHeader = bw.WriteReserve<PacketHeader>();

	bw.Write(lastTick);
	pktHeader->_type = PacketProtocol::S2C_LATENCY;
	pktHeader->_pktSize = bw.GetWriterSize();
	ThreadSafeSharedPtr safeSendBuffer = ThreadSafeSharedPtr(reinterpret_cast<PACKET_HEADER*>(sendBuffer), true);
	session->Send(safeSendBuffer);
}

void PacketHandler::HandlePacket_C2S_PLAYERCHAT(VillageServerConnection* connection, BYTE* packet, int32 packetSize)
{
	int32 chattingMsgSize;
	int32 sessionId = connection->GetConnectionId();
	int32 chatType;

	BinaryReader br(packet);
	WCHAR text[1000] = { 0 };

	br.Read(chatType);
	br.Read(chattingMsgSize);
	br.ReadWString(text, chattingMsgSize);
	;

	byte* sendBuffer = new byte[1000];
	BinaryWriter bw(sendBuffer);
	PacketHeader* pktHeader = bw.WriteReserve<PacketHeader>();

	bw.Write(chatType);
	bw.Write(sessionId);
	bw.Write(chattingMsgSize);
	bw.WriteWString(text, chattingMsgSize);

	pktHeader->_type = PacketProtocol::S2C_PLAYERCHAT;
	pktHeader->_pktSize = bw.GetWriterSize();

	ThreadSafeSharedPtr safeSendBuffer = ThreadSafeSharedPtr(reinterpret_cast<PACKET_HEADER*>(sendBuffer), true);

	switch (chatType)
	{
	case 0:
		MapManager::GetInstance()->BroadCast(connection->GetPlayer(), safeSendBuffer);
		break;

	case 1:
		ConnectionContext::GetInstance()->BroadCast(safeSendBuffer);
		break;
	}
}

void PacketHandler::HandlePacket_S2C_HEARTBIT(VillageServerConnection* session, BYTE* packet, int32 packetSize)
{
	session->SetHeartBeat();
}
// TODO 투기장에서만 할것
void PacketHandler::HandlePacket_C2S_PLAYERATTACK(VillageServerConnection* session, BYTE* packet, int32 packetSize)
{
	int32 otherPlayer;
	int32 damage;

	BinaryReader br(packet);
	br.Read(otherPlayer);
	br.Read(damage);

	Player* AttackPlayer = session->GetPlayer();
	Connection* AttackedSession = ConnectionContext::GetInstance()->GetConnection(otherPlayer);

	if (AttackedSession == nullptr)
		return;

	Player* AttackedPlayer = AttackedSession->GetPlayer();

	Vector3 attackedPos = AttackedPlayer->GetPos();
	Vector3 attackerPos = AttackPlayer->GetPos();

	if (attackedPos.x <= attackerPos.x + 2 && attackedPos.x >= attackedPos.x - 2)
	{
		if (attackedPos.z <= attackerPos.z + 2 && attackedPos.z >= attackedPos.z - 2)
		{
			AttackedPlayer->Attacked(session->GetPlayer(), damage);
		}
	}
}

void PacketHandler::HandlePacket_C2S_MONSTERATTACKED(VillageServerConnection* session, BYTE* packet, int32 packetSize)
{
	int32 monsterId;
	int32 damage;
	Vector3 monsterPos;

	BinaryReader br(packet);
	br.Read(monsterId);
	br.Read(monsterPos);
	br.Read(damage);

	Player* attacker = session->GetPlayer();
	Vector3 attackerPos = attacker->GetPos();

	MonsterManager::GetInstnace()->AttackedMonster(monsterId, attacker, damage);
}

void PacketHandler::HandlePacket_C2S_PLAYERESPAWN(VillageServerConnection* session, BYTE* packet, int32 packetSize)
{
	session->GetPlayer()->ReSpawn();
}
