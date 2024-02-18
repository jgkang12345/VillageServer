#include "pch.h"
#include "JGNet98App.h"
#include "ThreadManager.h"
#include "IOCPCore.h"
#include "ConnectionContext.h"
#include "VillageServerConnection.h"
#include "MapManager.h"
unsigned int _stdcall DispatchProc(void* Args)
{
	JGNet98App* app = reinterpret_cast<JGNet98App*>(Args);
	while (true)
		app->GetIOCPCore()->Dispatch();
}

int main() 
{
	PlayerDBConnectionPool::GetInstance()->Init(L"PLAYER", L"sa", L"root", 5);
	AccountDBConnectionPool::GetInstance()->Init(L"MSSQL", L"sa", L"root", 5);
	MapManager::GetInstance()->MapLoad("C:\\Users\\jgkang\\Desktop\\map\\VillageMap.dat");

	const char* ip = "127.0.0.1";
	uint16 port = 30004;
	
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	int32 threadCount = sysInfo.dwNumberOfProcessors * 2;

	JGNet98App villageServerApp(ip, port, VillageServerConnection::MakeGameSession);

	for (int i = 0; i < threadCount; i++)
		ThreadManager::GetInstacne()->Launch(DispatchProc, &villageServerApp);

	villageServerApp.Run(L"VillageServer");

	ThreadManager::GetInstacne()->AllJoin();
	return 0;
}