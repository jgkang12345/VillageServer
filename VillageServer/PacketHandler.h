#pragma once
class VillageServerConnection;

class PacketHandler
{
public:
	static void HandlePacket(VillageServerConnection* session, BYTE* packet, int32 packetSize);
private:
	static void HandlePacket_C2S_PLAYERINIT(VillageServerConnection* session, BYTE* dataPtr, int32 dataSize);
	static void HandlePacket_C2S_PLAYERSYNC(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_MAPSYNC(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_LATENCY(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_PLAYERCHAT(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_S2C_HEARTBIT(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_PLAYERATTACK(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_MONSTERATTACKED(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_PLAYERESPAWN(VillageServerConnection* session, BYTE* packet, int32 packetSize);
	static void HandlePacket_C2S_PLAYERSKILLSYNC(VillageServerConnection* session, BYTE* packet, int32 packetSize);

};

