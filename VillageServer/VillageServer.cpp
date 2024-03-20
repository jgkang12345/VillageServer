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
int32 Init() 
{
	PlayerDBConnectionPool::GetInstance()->Init(L"PLAYER", L"sa", L"root", 5);
	AccountDBConnectionPool::GetInstance()->Init(L"MSSQL", L"sa", L"root", 5);

	int32 serverPort;
	printf("30004~30006 마을, 30007~30008 초보자, 30009 중급, 30010~30011 고수\n");
	scanf_s("%d", &serverPort);	
	const char* ip = "58.236.130.58";

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
	MapManager::GetInstance()->MapLoadField("C:\\Users\\jgkang\\Desktop\\map\\HighFieldMap.dat");
}

void NoviceServerInit() 
{
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();
	MonsterTable::GetInstnace()->Init(con);
	PlayerDBConnectionPool::GetInstance()->Push(con);
	MonsterManager::GetInstnace()->Init(1000);
	MapManager::GetInstance()->MapLoadField("C:\\Users\\jgkang\\Desktop\\map\\NoviceFieldMap.dat");
}

void InterMediateServerInit()
{
	DBConnection* con = PlayerDBConnectionPool::GetInstance()->Pop();
	MonsterTable::GetInstnace()->Init(con);
	PlayerDBConnectionPool::GetInstance()->Push(con);
	MonsterManager::GetInstnace()->Init(1000);
	MapManager::GetInstance()->MapLoadField("C:\\Users\\jgkang\\Desktop\\map\\IntermediateFieldMap.dat");
}

void VillageServerInit() 
{
	MapManager::GetInstance()->MapLoad("C:\\Users\\jgkang\\Desktop\\map\\VillageMap.dat");
}

void Update(int32 currentTick) 
{
	MapManager::GetInstance()->Update(currentTick);
	MonsterManager::GetInstnace()->Update(currentTick);
}

int main() 
{
	const char* ip = "58.236.130.58";
	uint16 port = Init();
	
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	int32 threadCount = sysInfo.dwNumberOfProcessors * 2;

	JGNet98App villageServerApp(ip, port, VillageServerConnection::MakeGameSession);

	for (int i = 0; i < threadCount; i++)
		ThreadManager::GetInstacne()->Launch(DispatchProc, &villageServerApp);

	ThreadManager::GetInstacne()->Launch(HeartBitPingProc, nullptr);
	ThreadManager::GetInstacne()->Launch(AcceptProc, &villageServerApp);

	if (port != ServerPort::VILLAGE_SERVER)
	{
		while (true)
		{
			int32 currentTick = ::GetTickCount64();
			Update(currentTick);
			Sleep(200);
		}
	}

	ThreadManager::GetInstacne()->AllJoin();
	return 0;
}