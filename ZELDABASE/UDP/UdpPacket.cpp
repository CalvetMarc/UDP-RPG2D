#include "UdpPacket.h"
#include <iostream>
#include <cstring>

UdpPacket UdpPacket::Normal(PacketKey key, ICodable& codable)
{
	UdpPacket newPacket;
	newPacket << PacketType::NORMAL << key << codable;
	return newPacket;
}

UdpPacket UdpPacket::Critical(PacketKey key, ICodable& codable, CriticalPacketId& outId)
{
	UdpPacket newPacket;
	static CriticalPacketId newId = 0;
	newPacket << PacketType::CRITICAL << key << newId << codable;
	outId = newId;
	//DEBERES Evitar el Overflow
	newId++;
	return newPacket;
}

UdpPacket UdpPacket::CriticalResponse(PacketKey key, CriticalPacketId criticalPacketId, ICodable& codable)
{
	UdpPacket newPacket;
	newPacket << PacketType::CRITICALRESPONSE << key << criticalPacketId << codable;
	return newPacket;
}

UdpPacket UdpPacket::Acumulated(CPVector<UdpPacket> packetsList)
{
	UdpPacket newPacket;
	newPacket << PacketType::ACUMULATED << packetsList;
	return newPacket;
}

void UdpPacket::Code(sf::Packet& packet)
{
	size_t size = this->getDataSize();
	packet << size;
	packet.append(this->getData(), size);
}

void UdpPacket::Decode(sf::Packet& packet)
{
	size_t size;
	packet >> size;

	//Leemos exactamente la información que queremos de forma manual
	const void* fullData = packet.getData();
	const char* fullDataChar = static_cast<const char*>(fullData);
	void* newData = malloc(size);

	//Precaución para evitar el mensaje de alerta de:
	//https://learn.microsoft.com/en-us/cpp/code-quality/c6387?view=msvc-170
	if (newData != nullptr)
	{
		std::memcpy(newData, fullDataChar + packet.getReadPosition(), size);

		//Forzamos al puntero del packete que es privado a moverse
		for (size_t i = 0; i < size; i++)
		{
			bool unit;
			packet >> unit;
		}

		this->append(newData, size);

		//No hace falta liberar esta memoria, porque es un putero que controla el pakete
		//delete fullData;
		//const void* fullData2 = packet.getData();
		//delete fullDataChar; No hay que borrarlo porque solo es
		//Esta es una buena oportunidad para enseñar a usar el debuguer de memoria
		free(newData);
	}	
}
