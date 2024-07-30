#pragma once
#include <map>
#include <functional>
#include <string>
#include "Utils.h"
#include <vector>
#include <thread>
#include <mutex>

const float subTickTime = 15.625f; //64 times per second
const float playersVelocity = 40.f;
const float playersSwordKnockbackForce = 10;
const sf::Uint32 gameDurationInMs = 60000; //60000ms = 1min

class Server
{
public:
	Server();
	~Server();
	void InitServer();
private:
	sf::UdpSocket* udpSocket; //Server socket
	std::atomic<int>playersID, bombsID; //Id que sassignen a tots els players i bombes
	std::map<CommunicationKeys, std::function<void(sf::Packet, OnlineClient*)>> commandMap; //Cada key d'un paquete te la seva accio associada
	std::map<int, OnlineClient*> clientsPlaying; //Map dels clients de la partida i la seva id
	std::map<int, OnlineBomb*> bombsInGame; //Map de les bombes de la partida i la seva id
	std::mutex clientsPlayingMutex, bombsInGameMutex, playersIdMutex, messagesHistoryMutex; //Mutex de les zones critiques
	std::atomic<sf::Uint32> elapsedTime, lastElapsedTime; //Variables per controlar el temps, la diferencia entre ellas es el deltaTime
	sf::Uint32 subTick, timeRemaining; //Contador per actualitzar el joc
	std::thread* threadReceive;
	std::atomic<bool> sesionEnd;
	CPVector<ChatMessage> messagesHistory;
	void ExecuteCommand(CommunicationKeys key, sf::Packet data, OnlineClient* clientWhoSentMessage); //Crida la accio que toca depenent de la key que sha rebut al paquet
	void HandleServerReceiveData(); //Bucle per rebre paquets
	void HandleGameEvents(); //Bucle d'actualitzar l'estat de la partida
	void SendData(int clientID, sf::Packet p); //Per enviar info a un player en concret
	void SendDataAll(sf::Packet p); //Per enviar info a tots els players de la partida
	void SendDataAllButOne(int clientID, sf::Packet p); //Per enviar info a tots els players de la partida menys un
	void SpawnBomb(); //Crea una nova bomba a la partida
	sf::Uint32 GetTime();
	OnlineClient* GetNewClient(sf::IpAddress ip, unsigned short port, sf::Packet* p); //Crea un nou player a la partida i el retorna
	OnlineClient* GetClient(sf::Packet* p); //Retorna un player segons la seva id
	void AskForInputs(); //Envia un paquet als clients per dirlsi que enviin tot el seu input buffer
	void CheckGameElementsState(); //Comprova l'estat de tots els elements del joc
};

