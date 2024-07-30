#pragma once
#include "UdpConnection.h"

class UdpSocket: sf::UdpSocket, public UdpSendPacketDelegate
{
public:
	typedef std::function<void(UdpConnection* connection)> OnConnectionEnter;
public:

	UdpSocket(UdpAddress::PortNumber port, OnConnectionEnter onConectionEnter);
	~UdpSocket();

	void ConnectTo(UdpAddress address);

	UdpAddress GetAddress();
private:

	bool _isRunning = false;
	std::mutex _isRunningMutex;

	OnConnectionEnter _onConnectionEnter;
	UdpAddress _address;

	const UdpPacket::PacketKey CONNECTIONKEY = 0;
	std::map<std::string, UdpConnection*> _connectionsMap;
	std::map<std::string, UdpConnection*> _pendantConnectionsMap;

	void ReceiveLoop();

	void ManageReceivedPacketDone(UdpPacket packet, UdpAddress address);

	virtual void SendAccumulating(UdpAddress address, UdpPacket packet) override;
	virtual void SendImmediately(UdpAddress address, UdpPacket packet) override;
};

