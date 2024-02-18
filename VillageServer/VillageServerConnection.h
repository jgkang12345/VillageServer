#pragma once
#include "Connection.h"
class VillageServerConnection : public Connection
{
private:
	WCHAR _username[50] = {};
	int32 _lastServerPort;

public:
	static Connection* MakeGameSession(const SOCKET& socket, const SOCKADDR_IN& sockAddr) { return new VillageServerConnection(socket, sockAddr); }

public:
	VillageServerConnection(const SOCKET& socket, const SOCKADDR_IN& sockAddrIn);
	virtual ~VillageServerConnection();

public:
	virtual void	OnRecv(Connection* connection, byte* dataPtr, int32 dataLen);
	virtual void	OnDisconnect();
	virtual void    OnConnect();
};

