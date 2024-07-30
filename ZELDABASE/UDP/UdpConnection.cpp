#include "UdpConnection.h"
#include <thread>
#include <windows.h>

UdpConnection::UdpConnection(UdpSendPacketDelegate* delegate, UdpAddress address)
{
	this->_delegate = delegate;
	this->_address = address;
	std::thread* reSendLoopThread = new std::thread(&UdpConnection::ReSendCriticalLoop, this);
	reSendLoopThread->detach();
}

UdpConnection::~UdpConnection()
{
}

UdpAddress UdpConnection::GetAddress()
{
	return _address;
}

void UdpConnection::Send(UdpPacket::PacketKey key, ICodable& codable)
{
	this->_delegate->SendImmediately(_address, UdpPacket::Normal(key, codable));
}

void UdpConnection::Subscribe(UdpPacket::PacketKey key, OnReceivePacket onReceivePacket)
{
	_subscriptionsMutex.lock();
	_subscriptions[key] = onReceivePacket;
	_subscriptionsMutex.unlock();
}

void UdpConnection::SubscriveAsync(UdpPacket::PacketKey key, OnReceivePacket onReceivePacket)
{
	std::thread* subscriveThread = new std::thread(&UdpConnection::Subscribe, this, key, onReceivePacket);
	subscriveThread->detach();
}

void UdpConnection::SendCritical(UdpPacket::PacketKey key, ICodable& codable, OnReceiveCriticalResponse onResponse)
{
	UdpPacket::CriticalPacketId outId;
	UdpPacket criticalPacket = UdpPacket::Critical(key, codable, outId);//TODO Por ahora el id es unico y auto incremental pero hay que mejorarlo
	UdpCriticalPerseverator perseverator = UdpCriticalPerseverator(criticalPacket, onResponse);
	_criticalResponsesSubscriptionsMutex.lock();
	_criticalResponsesSubscriptions[outId] = perseverator;
	_criticalResponsesSubscriptionsMutex.unlock();
	this->_delegate->SendImmediately(_address, criticalPacket);
}

void UdpConnection::SubscribeOnCritical(UdpPacket::PacketKey key, OnReceiveCriticalPacket onReceiveCriticalPacket)
{
	_criticalSubscriptionsMutex.lock();
	_criticalSubscriptions[key] = onReceiveCriticalPacket;
	_criticalSubscriptionsMutex.unlock();
}

void UdpConnection::SubscriveOnCriticalAsync(UdpPacket::PacketKey key, OnReceiveCriticalPacket onReceiveCriticalPacket)
{
	std::thread* subscriveThread = new std::thread(&UdpConnection::SubscribeOnCritical, this, key, onReceiveCriticalPacket);
	subscriveThread->detach();
}

void UdpConnection::ManageReceivedPacket(UdpPacket packet)
{
	UdpPacket::PacketType type;
	sf::Uint8 intType;
	packet >> intType;
	type = static_cast<UdpPacket::PacketType>(intType);

	switch (type)
	{
	case UdpPacket::NORMAL:
		ManageNormal(packet);
		break;
	case UdpPacket::CRITICAL:
		ManageCritical(packet);
		break;
	case UdpPacket::CRITICALRESPONSE:
		ManageCriticalResponse(packet);
		break;
	case UdpPacket::ACUMULATED:
		ManageAcumulated(packet);
		break;
	default:
		//Error No Debería suceder
		break;
	}
}

void UdpConnection::ManageNormal(UdpPacket packet)
{
	UdpPacket::PacketKey key;
	packet >> key;

	std::map<UdpPacket::PacketKey, OnReceivePacket>::iterator it;

	_subscriptionsMutex.lock();

	it = _subscriptions.find(key);
	if (it != _subscriptions.end())
	{
		it->second(packet);
	}

	_subscriptionsMutex.unlock();
}

void UdpConnection::ManageCritical(UdpPacket packet)
{
	UdpPacket::PacketKey key;
	UdpPacket::CriticalPacketId id;
	packet >> key >> id;

	std::map<UdpPacket::PacketKey, OnReceiveCriticalPacket>::iterator it;

	_criticalSubscriptionsMutex.lock();

	it = _criticalSubscriptions.find(key);
	if (it != _criticalSubscriptions.end())
	{
		ICodable* codableResponse = it->second(packet);
		UdpPacket critiaclResponse = UdpPacket::CriticalResponse(key, id, *codableResponse);
		_delegate->SendImmediately(GetAddress(), critiaclResponse);
		delete codableResponse;
	}

	_criticalSubscriptionsMutex.unlock();
}

void UdpConnection::ManageCriticalResponse(UdpPacket packet)
{
	UdpPacket::PacketKey key;
	UdpPacket::CriticalPacketId id;
	packet >> key >> id;

	std::map<UdpPacket::CriticalPacketId, UdpCriticalPerseverator>::iterator it;

	_criticalResponsesSubscriptionsMutex.lock();

	it = _criticalResponsesSubscriptions.find(id);
	if (it != _criticalResponsesSubscriptions.end())
	{
		it->second.onReceiveCriticalResponse(packet);
		_criticalResponsesSubscriptions.erase(it);
		//_criticalResponsesSubscriptions.erase(it); //Remove with iterator is best
	}

	_criticalResponsesSubscriptionsMutex.unlock();
}

void UdpConnection::ManageAcumulated(UdpPacket packet)
{
	CPVector<UdpPacket> packets;

	packet >> packets;

	for (UdpPacket* nextPacket : packets)
	{
		this->ManageReceivedPacket(*nextPacket);
	}

	//Una vez procesados los packetes no se necesitan para nada mas, lo dejo comentado para evitar problemas por ahora
	for (UdpPacket* nextPacket : packets)
	{
		delete nextPacket;
	}
}

void UdpConnection::ReSendCriticalLoop()
{
	while (this != nullptr)
	{
		Sleep(this->_reSendFrequency);


		UdpAddress address = GetAddress();

		_criticalResponsesSubscriptionsMutex.lock();

		for (auto it : _criticalResponsesSubscriptions)
		{
			_delegate->SendImmediately(address,it.second.packet);
		}

		_criticalResponsesSubscriptionsMutex.unlock();
	}
}
