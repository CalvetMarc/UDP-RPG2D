#include "UdpSocket.h"
#include <thread>
#include <iostream>

class ConnectionHandShake: public ICodable
{
public:

	UdpAddress address;
	bool isHello;

	ConnectionHandShake() {};

	ConnectionHandShake(UdpAddress address, bool isHello) {
		this->address = address;
		this->isHello = isHello;
	}

	virtual void Code(sf::Packet& packet) override 
	{
		packet << address << isHello;
	}

	virtual void Decode(sf::Packet& packet) override 
	{
		packet >> address >> isHello;
	}
};

UdpSocket::UdpSocket(UdpAddress::PortNumber port, OnConnectionEnter onConectionEnter)
{
	this->_onConnectionEnter = onConectionEnter;

	sf::IpAddress ipAddress = sf::IpAddress::getLocalAddress();
	this->_address = UdpAddress(ipAddress.toString(), port);

	if (this->bind(port) != sf::Socket::Done)
	{
		//TODO Manage Error
	}
	else
	{
		std::thread* loopThread = new std::thread(&UdpSocket::ReceiveLoop, this);
		loopThread->detach();
	}
}

UdpSocket::~UdpSocket()
{
}

void UdpSocket::ConnectTo(UdpAddress address)
{
	ConnectionHandShake helloHandShake = ConnectionHandShake(GetAddress(), true);
	UdpConnection* newConnection = new UdpConnection(this, address);

	_pendantConnectionsMap[newConnection->GetAddress().ToString()] = newConnection;
	newConnection->SendCritical(CONNECTIONKEY, helloHandShake, [this, newConnection](UdpPacket response) {

		std::string key = newConnection->GetAddress().ToString();
		this->_connectionsMap[key] = newConnection;
		this->_pendantConnectionsMap.erase(key);
		this->_onConnectionEnter(newConnection);
		});
}

UdpAddress UdpSocket::GetAddress()
{
	return this->_address;
}

void UdpSocket::ReceiveLoop()
{
	_isRunning = true;
	bool isRunning = true;

	do
	{
		UdpPacket packet;
		sf::IpAddress ip;
		unsigned short port;

		sf::Socket::Status status = this->receive(packet, ip, port);

		UdpAddress address = UdpAddress(ip.toString(), port);

		switch (status)
		{
		case sf::Socket::Done:
			std::cout << "Packet Received Done From: " << address.ToString() << std::endl;
			ManageReceivedPacketDone(packet, address);
			break;
		case sf::Socket::Partial:
			std::cout << "Packet Received Partial From: " << address.ToString() << std::endl;
			break;
		case sf::Socket::NotReady:
			std::cout << "Packet Received NotReady From: " << address.ToString() << std::endl;
			break;
		case sf::Socket::Disconnected:
			std::cout << "Packet Received Disconnected From: " << address.ToString() << std::endl;
			break;
		case sf::Socket::Error:
			std::cout << "Packet Received Error From: " << address.ToString() << std::endl;
			break;
		default:
			break;
		}

		_isRunningMutex.lock();
		isRunning = _isRunning;
		_isRunningMutex.unlock();
	} while (isRunning);
	
}

void UdpSocket::ManageReceivedPacketDone(UdpPacket packet, UdpAddress address)
{
	std::map<std::string, UdpConnection*>::iterator it;

	std::string addressKey = address.ToString();

	it = _connectionsMap.find(addressKey);
	if (it != _connectionsMap.end())
	{
		//Connection Exist
		it->second->ManageReceivedPacket(packet);
	}
	else
	{
		std::map<std::string, UdpConnection*>::iterator itPendant;
		it = _pendantConnectionsMap.find(addressKey);

		if (it != _pendantConnectionsMap.end())
		{
			it->second->ManageReceivedPacket(packet);
		}
		else
		{
			//Connection Is New
			//Todo Combertir esto en un packete critico para asegurase que el welcome le llega

			sf::Uint8 intType;
			sf::Uint8 intKey;
			UdpPacket::CriticalPacketId id;
			ConnectionHandShake handShake;

			packet >> intType >> intKey >> id >> handShake;

			if (handShake.address.ToString() == addressKey && handShake.isHello)
			{
				UdpConnection* newConnection = new UdpConnection(this, address);
				_connectionsMap[addressKey] = newConnection;

				newConnection->SubscribeOnCritical(CONNECTIONKEY, [](UdpPacket helloPacket) {

						ConnectionHandShake* handShake = new ConnectionHandShake();
						helloPacket >> *handShake;
						handShake->isHello = false;

						return handShake;
					});

				ConnectionHandShake welcomeHandShake = ConnectionHandShake(GetAddress(), false);
				SendImmediately(address, UdpPacket::CriticalResponse(intKey, id, welcomeHandShake));

				_onConnectionEnter(newConnection);
			}
		}
	}
}

void UdpSocket::SendAccumulating(UdpAddress address, UdpPacket packet)
{

}

void UdpSocket::SendImmediately(UdpAddress address, UdpPacket packet)
{
	std::cout << "Sending Packet To: " << address.ToString();
	this->send(packet, address.ip, address.port);
}
