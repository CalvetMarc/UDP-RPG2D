#pragma once
#include <functional>
#include <mutex>
#include <map>

#include "UdpAddress.h"
#include "UdpPacket.h"

class UdpSendPacketDelegate
{
public:

	virtual void SendAccumulating(UdpAddress address, UdpPacket packet) = 0;
	virtual void SendImmediately(UdpAddress address, UdpPacket packet) = 0;

};

class UdpConnection
{
public:
	typedef std::function<void(UdpPacket packet)> OnReceivePacket;
	typedef std::function<ICodable*(UdpPacket packet)> OnReceiveCriticalPacket;
	typedef std::function<void(UdpPacket packet)> OnReceiveCriticalResponse;
private:
	class UdpCriticalPerseverator {
	public:
		UdpPacket packet;
		OnReceiveCriticalResponse onReceiveCriticalResponse;
		UdpCriticalPerseverator() {};
		UdpCriticalPerseverator(UdpPacket packet, OnReceiveCriticalResponse onReceiveCriticalResponse)
		{
			this->packet = packet;
			this->onReceiveCriticalResponse = onReceiveCriticalResponse;
		}
	};
public:

	UdpConnection(UdpSendPacketDelegate* delegate, UdpAddress address);
	~UdpConnection();

	UdpAddress GetAddress();

	void Send(UdpPacket::PacketKey key, ICodable& codable);
	void Subscribe(UdpPacket::PacketKey key, OnReceivePacket onReceivePacket);
	void SubscriveAsync(UdpPacket::PacketKey key, OnReceivePacket onReceivePacket);

	void SendCritical(UdpPacket::PacketKey key, ICodable& codable, OnReceiveCriticalResponse onResponse);
	void SubscribeOnCritical(UdpPacket::PacketKey key, OnReceiveCriticalPacket onReceiveCriticalPacket);
	void SubscriveOnCriticalAsync(UdpPacket::PacketKey key, OnReceiveCriticalPacket onReceiveCriticalPacket);

	void ManageReceivedPacket(UdpPacket packet);
private:
	
	UdpSendPacketDelegate* _delegate;
	UdpAddress _address;

	std::map<UdpPacket::PacketKey, OnReceivePacket> _subscriptions;
	std::mutex _subscriptionsMutex;

	std::map<UdpPacket::PacketKey, OnReceiveCriticalPacket> _criticalSubscriptions;
	std::mutex _criticalSubscriptionsMutex;

	unsigned long _reSendFrequency = 100;
	std::map<UdpPacket::CriticalPacketId, UdpCriticalPerseverator> _criticalResponsesSubscriptions;
	std::mutex _criticalResponsesSubscriptionsMutex;

	void ManageNormal(UdpPacket packet);
	void ManageCritical(UdpPacket packet);
	void ManageCriticalResponse(UdpPacket packet);
	void ManageAcumulated(UdpPacket packet);

	void ReSendCriticalLoop();
};

