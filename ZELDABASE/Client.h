#pragma once
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "ResourcesClass/Player.h"
#include <future>

class Player;

class Game {	

public:
	bool close = false;
	sf::Vector2f playerSpawn;
	Game(int _idClient, std::string _name, std::atomic<bool>* _sessionEnd, sf::Uint32 _time, sf::Uint32 _gameDuration, Player*_player, std::function<bool()> _OnDisconnect, std::function<bool(sf::Packet)> _SendDataFunc);
	~Game();
	Player* GetPlayer() const;
	void HandleEntityUpdate(OnlineClient oc); //Actualitza un altre jugador de la partida
	void HandleBombUpdate(OnlineBomb ob); //Actualitza una bomba de la partida
	void HandleLoadEntities(CPVector<OnlineClient> gameEntities); //Carrega els jugadors de la partida
	void HandleLoadBombs(CPVector<OnlineBomb> gameBombs); //Carrega les bombes de la partida
	void EntityLeft(int idLeft); //Per treure un jugador de la partida
	void EntityDead(int idLeft); //Perquee s vegi la mort d'un jugador
	void ExplodeBomb(int id); //Perque exploti una bomba
	void PointsGained(int pointsToAdd);
	void LoadMessage(ChatMessage mssg);
	void LoadMessageHistory(CPVector<ChatMessage> mssgs);
	void SetResults(CPVector<GamePoints> results);
	void Update(); //Update del joc
private:

	struct OnlinePlayer {
		OnlinePlayer() = default;
		OnlinePlayer(OnlineClient* _playerClient, Animation* _playerCurrentAnimation, LinkCharacter* _playerCharacter) : playerClient(_playerClient), playerCurrentAnimation(_playerCurrentAnimation), playerCharacter(_playerCharacter) {}
		~OnlinePlayer() {
			delete playerClient;
			playerClient = nullptr;
			delete playerCurrentAnimation;
			playerCurrentAnimation = nullptr;
			delete playerCharacter;
			playerCharacter = nullptr;
		}
		OnlineClient* playerClient;
		Animation* playerCurrentAnimation;
		LinkCharacter* playerCharacter;
	};

	std::mutex playersMutex, bombsMutex, playersRemoveMutex, resultsMutex, gameMssgsMutex;
	std::function<bool()> OnDisconnect; //Funcio a fer quan el client demana desonectarse tancant la pantalla
	std::function<bool(sf::Packet)> SendDataFunc;
	Player* player;
	sf::Font font;
	std::map<int, OnlinePlayer*> playersInGame; //Tots els players de la partida
	std::map<int, OnlineBomb*> bombsInGame; //Totes les bombes de la partida
	std::map<int, sf::Uint32> playersPendingRemove; //Players que estan fent animacio de morir i s'hauran de borrar quan acabin
	float playersVelocity;
	sf::Uint32 connectionTime, elapsedTime, lastElapsedTime, timeRemaining, gameDuration; //Variables de temps, elapsed es conenction time mes suma de deltaTimes, que es la diferenica entre elapsed i lastElapsed
	sf::RenderWindow* gameWindow; //Pantalla del joc
	std::atomic<bool>* sessionEnd, gameEnd; //Per controlar si s'ha acabat la partida (Thas desconectat o than matat)
	std::vector<std::pair<int, sf::Uint32>> bombsInCountown; //Bombes que han sigut llençades i estan apunt dexplotar
	std::multimap<int, std::string> gameResults;
	bool gameMode = true;
	std::string input, name;
	CPVector<ChatMessage> gameMessages;
	void Render(); //Pintar la pantalla
	void HandleWindow(sf::Uint16 deltaTime); //Controla els inputs
	void HandleEntites(sf::Uint16 deltaTime); //Controla el estat dels players
	void HandleBombs(sf::Uint16 deltaTime); //Controla el estat de les bombes
	void HandleTimeCounter(sf::Uint16 deltaTime);
	sf::Uint32 GetTime();
	std::string GetMssgTime(sf::Uint32 timeStamp);
	sf::RectangleShape* backgroundRect, *backgroundChatRect, * backgroundChatDepthRect, * backgroundInputRect;
	int idClient; 
	sf::Text pointsText, timeRemainingText, resultsText, inputText, mssgText;
	std::atomic<int> points;
};

class Client
{
public:
	Client();
	~Client();
	void ConnectToServer(); //Envia al servidor que ens volem conectar
private:
	Game* game;
	Player* player;
	std::atomic<bool> sessionEnd;
	std::string name; //Nom que ha introduit el client
	sf::UdpSocket* udpSocket; //Client socket
	std::map<CommunicationKeys, std::function<void(sf::Packet)>> commandMap; //Cada key d'un paquete te la seva accio associada
	std::thread* threadReceive; //Thread que rep els paquets
	std::atomic<int> id; //Id que el servidor ha donat al client
	std::atomic<bool> gameInitialized; //Flag per saber si ya s'ha carregat la partida
	sf::Uint32 connectionTime; //Temps real de la partida quan ens hem conectat
	sf::Uint32 gameDuration; //Temps q dura la partida
	std::mutex clientLoggedMutex, gameInitMutex;
	std::condition_variable clientLogged; //Per controlar si ya ens hem logejat
	std::condition_variable gameInit; //Per controlar si la partida ya s'ha carregat
	void HandleClientReceiveData(); //Rep la info del servidor
	void HandleGameInterface(); //Crea la partida i porta el seu bucle
	bool SendData(sf::Packet p); //Envia info al server
	void ExecuteCommand(CommunicationKeys key, sf::Packet data); //Crida la accio que toca depenent de la key que sha rebut al paquet
};

