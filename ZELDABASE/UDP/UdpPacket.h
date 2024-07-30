#pragma once
#include <SFML/Network.hpp>
#include "../ICodable.h"

class UdpPacket: public sf::Packet, public ICodable
{
public:
	typedef sf::Uint8 PacketKey;
	typedef sf::Uint64 CriticalPacketId;
public:
	enum PacketType: sf::Uint8
	{ 
		NORMAL = 0,
		CRITICAL = 1,
		CRITICALRESPONSE = 2,
		ACUMULATED = 3
	};

	static UdpPacket Normal(PacketKey key, ICodable& codable);
	static UdpPacket Critical(PacketKey key, ICodable& codable, CriticalPacketId& outId);
	static UdpPacket CriticalResponse(PacketKey key, CriticalPacketId criticalPacketId, ICodable& codable);
	static UdpPacket Acumulated(CPVector<UdpPacket> packetsList);

	virtual void Code(sf::Packet& packet) override;
	virtual void Decode(sf::Packet& packet) override;
	
private:

};

