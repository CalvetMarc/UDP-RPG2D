#pragma once
#include <string>
#include "../ICodable.h"

class UdpAddress: public ICodable
{
public:
	typedef unsigned short PortNumber;
public:

	UdpAddress();
	UdpAddress(std::string ip, PortNumber port);
	~UdpAddress();

	std::string ip = "";
	PortNumber port = 0;

	std::string ToString();



	// Heredado vía ICodable
	virtual void Code(sf::Packet& packet) override;

	virtual void Decode(sf::Packet& packet) override;

};