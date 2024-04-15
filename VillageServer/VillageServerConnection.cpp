#include "pch.h"
#include "VillageServerConnection.h"
#include "ConnectionContext.h"
#include "PacketHandler.h"
#include "MapManager.h"
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
	Connection::OnDisconnect();
}

void VillageServerConnection::OnConnect()
{
}
