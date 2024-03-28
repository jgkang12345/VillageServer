#include "pch.h"
#include "JGNet98App.h"
#include "ThreadManager.h"
#include "IOCPCore.h"
#include "ConnectionContext.h"
#include "VillageServerConnection.h"
#include "MapManager.h"
#include "MonsterTable.h"
#include "MonsterManager.h"

unsigned int _stdcall DispatchProc(void* Args)
{
	JGNet98App* app = reinterpret_cast<JGNet98App*>(Args);
	while (true)
		app->GetIOCPCore()->Dispatch();
}

unsigned int _stdcall HeartBitPingProc(void* Args) 
{
	while (true)
	{
		ConnectionContext::GetInstance()->HeartBeatPing();
		Sleep(5000);
	}
}

unsigned int _stdcall AcceptProc(void* Args)
{
	JGNet98App* app = reinterpret_cast<JGNet98App*>(Args);
	app->Run(L"Server Start");
	return 0;
}


void NoviceServerInit();
void VillageServerInit();
void InterMediateServerInit();
void HightServerInit();
int32 Init(int32 serverPort)
{
	PlayerDBConnectionPool::GetInstance()->Init(L"PLAYER", L"sa", L"root", 5);
	AccountDBConnectionPool::GetInstance()->Init(L"MSSQL", L"sa", L"root", 5);

	// int32 serverPort;
	// printf("30004~30006 마을, 30007~30008 초보자, 30009 중급, 30010~30011 고수\n");
	// scanf_s("%d", &serverPort);	
	const char* ip = "58.236.130.58";
	printf("ServerPort: %d\n", serverPort);
	switch (serverPort)
	{
	case 30007:
	case 30008:
		NoviceServerInit();
		break;

	case 30004:
	case 30005:
	case 30006:
		VillageServerInit();
		break;

	case 30009:
		InterMediateServerInit();
		break;

	case 30010:
	case 30011:
		HightServerInit();
		break;
	}

	return serverPort;
}

void HightServerInit() 
{
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();
	MonsterTable::GetInstnace()->Init(con);
	PlayerDBConnectionPool::GetInstance()->Push(con);
	MonsterManager::GetInstnace()->Init(1000);
	MapManager::GetInstance()->MapLoadField(ServerType::HIGH,"map\\HighFieldMap.dat");
}

void NoviceServerInit() 
{
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();
	MonsterTable::GetInstnace()->Init(con);
	PlayerDBConnectionPool::GetInstance()->Push(con);
	MonsterManager::GetInstnace()->Init(1000);
	MapManager::GetInstance()->MapLoadField(ServerType::NOVICE, "map\\NoviceFieldMap.dat");
}

void InterMediateServerInit()
{
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();
	MonsterTable::GetInstnace()->Init(con);
	PlayerDBConnectionPool::GetInstance()->Push(con);
	MonsterManager::GetInstnace()->Init(1000);
	MapManager::GetInstance()->MapLoadField(ServerType::INTERMEDIATE, "map\\IntermediateFieldMap.dat");
}

void VillageServerInit() 
{
	MapManager::GetInstance()->MapLoad(ServerType::VILLAGE, "map\\VillageMap.dat");
}

void Update(int32 currentTick) 
{
	ServerType type = MapManager::GetInstance()->GetServerType();
	
	if (type == ServerType::VILLAGE)
		return;
	
	MapManager::GetInstance()->Update(currentTick);
	MonsterManager::GetInstnace()->Update(currentTick);
}

SOCKET connectSocket;	SOCKADDR_IN serverSockAddrIn;
bool MonitorInit(int32 port) 
{
	const char* monitorIp = "58.236.130.58";
	int monitorPort = 7777;

	connectSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (connectSocket == INVALID_SOCKET)
		return false;

	serverSockAddrIn.sin_family = AF_INET;
	inet_pton(AF_INET, monitorIp, &serverSockAddrIn.sin_addr);
	serverSockAddrIn.sin_port = htons(7777);

	if (connect(connectSocket, reinterpret_cast<SOCKADDR*>(&serverSockAddrIn), sizeof(serverSockAddrIn)) == SOCKET_ERROR)
	{
		closesocket(connectSocket);
		return false;
	}

	byte sendBuffer[100] = {};
	BinaryWriter bw(sendBuffer);
	PacketHeader* header = bw.WriteReserve<PacketHeader>();
	header->_type = S2C_MONITORINIT;
	bw.Write(port);
	header->_pktSize = bw.GetWriterSize();
	::send(connectSocket, reinterpret_cast<char*>(sendBuffer), bw.GetWriterSize(), 0);
	return true;
}

int main(int argc, char* argv[])
{
	const char* ip = "58.236.130.58";
	int data = std::atoi(argv[1]);
	uint16 port = Init(data);
	
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	int32 threadCount = sysInfo.dwNumberOfProcessors * 2;

	JGNet98App villageServerApp(ip, port, VillageServerConnection::MakeGameSession);

	bool monitorCon = MonitorInit(port);

	if (!monitorCon)
	{
		printf("모니터 연결 실패\n");
		return false;
	}


	for (int i = 0; i < threadCount; i++)
		ThreadManager::GetInstacne()->Launch(DispatchProc, &villageServerApp);

	ThreadManager::GetInstacne()->Launch(HeartBitPingProc, nullptr);
	ThreadManager::GetInstacne()->Launch(AcceptProc, &villageServerApp);

	int32 sumTick = 0;
	while (true)
	{
		int32 currentTick = ::GetTickCount64();
		Update(currentTick);
		Sleep(200);
		sumTick += 200;
		if (sumTick >= 1000) 
		{
			int32 connectionCnt = ConnectionContext::GetInstance()->GetConnectionCnt();
			byte sendBuffer[100] = {};
			BinaryWriter bw(sendBuffer);

			PacketHeader* header = bw.WriteReserve<PacketHeader>();
			bw.Write(connectionCnt);
			header->_type = S2C_CONNECTIONLIST;
			header->_pktSize = bw.GetWriterSize();
			::send(connectSocket, reinterpret_cast<char*>(sendBuffer), bw.GetWriterSize(), 0);
			sumTick -= 1000;
		}
	}

	ThreadManager::GetInstacne()->AllJoin();
	return 0;
}