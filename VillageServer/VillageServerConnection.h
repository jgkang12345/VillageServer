#pragma once
#include "Connection.h"
class VillageServerConnection : public Connection
{
private:
	WCHAR _username[50] = {};
	int32 _lastServerPort;

public:
	static Connection* MakeGameSession(const SOCKET& socket, const SOCKADDR_IN& sockAddr) { return new VillageServerConnection(socket, sockAddr); }
	static Connection* MakeGameSession2() { return new VillageServerConnection(); }

public:
	VillageServerConnection(const SOCKET& socket, const SOCKADDR_IN& sockAddrIn);
	VillageServerConnection();
	virtual ~VillageServerConnection();

public:
	virtual void	OnRecv(Connection* connection, byte* dataPtr, int32 dataLen);
	virtual void	OnDisconnect();
	virtual void    OnConnect();
};

