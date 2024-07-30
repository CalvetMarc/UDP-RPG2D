#include "Client.h"
#include <iostream>
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

Client::Client()
{
	id.store(-1);
	sessionEnd.store(false);
	gameInitialized.store(false);

	udpSocket = new sf::UdpSocket();
	udpSocket->setBlocking(true);

	game = nullptr;

	commandMap.insert({ CommunicationKeys::Login, [this](sf::Packet packet) { //Ens guardem la id, temps de partida, velocitat i poiscio i que ens ha donat el server al conectarnos
		sf::Uint32 auxTime;
		float auxVel, posX, posY;
		int auxId;
		packet >> auxId;
		packet >> auxTime;
		packet >> auxVel;
		packet >> posX;
		packet >> posY;
		packet >> gameDuration;
		connectionTime = auxTime;		
		player = new Player(sf::Vector2u(posX, posY), auxVel, name);
		std::unique_lock<std::mutex> lock(clientLoggedMutex);
		id.store(auxId);	
		clientLogged.notify_one();
	}});	

	commandMap.insert({ CommunicationKeys::Inputs, [this](sf::Packet packet) { //Quan el server ho demana, li enviem tots els inputs que hem fet desde l'ultim cop EX:6 //EX:6.2
		sf::Packet p;
		p << static_cast<int>(CommunicationKeys::Inputs) << id;		
		if (player != nullptr) {
			if(player->ProcessInputs(p))
				/*if (*/SendData(p)/*)
					std::cout << "Inputs Sended" << std::endl*/;
		}
		
	}});

	commandMap.insert({ CommunicationKeys::PosFixed, [this](sf::Packet packet) { //Actualitzem la posicio del nostre player quan el servidor ens la corregeix
		sf::Vector2f newPos;
		packet >> newPos.x >> newPos.y;
		game->GetPlayer()->SetPos(newPos);
	}});

	commandMap.insert({ CommunicationKeys::Message, [this](sf::Packet packet) { //Actualitzem la posicio del nostre player quan el servidor ens la corregeix
		ChatMessage mssg;
		packet >> mssg;
		std::mutex myMutex;
		std::unique_lock<std::mutex> lock(myMutex);
		gameInit.wait(lock, [this] {return gameInitialized.load(); }); //Esperem a que s'hagi creat la partida
		game->LoadMessage(mssg);
	} });

	commandMap.insert({ CommunicationKeys::GameEnded, [this](sf::Packet packet) { //Actualitzem la posicio del nostre player quan el servidor ens la corregeix
		CPVector<GamePoints> gameResultsCP;

		std::mutex myMutex;
		std::unique_lock<std::mutex> lock(myMutex);
		gameInit.wait(lock, [this] {return gameInitialized.load(); }); //Esperem a que s'hagi creat la partida

		packet >> gameResultsCP;
		game->SetResults(gameResultsCP);
	} });

	commandMap.insert({ CommunicationKeys::Points, [this](sf::Packet packet) { //Actualitzem la posicio del nostre player quan el servidor ens la corregeix
		int points;
		packet >> points;
		game->PointsGained(points);
	} });

	commandMap.insert({ CommunicationKeys::PlayerUpdate, [this](sf::Packet packet) { //actualizem un player que ens ha dit el servidor
		OnlineClient oc;
		packet >> oc;

		std::mutex myMutex;
		std::unique_lock<std::mutex> lock(myMutex);
		gameInit.wait(lock, [this] {return gameInitialized.load(); }); //Esperem a que s'hagi creat la partida
		game->HandleEntityUpdate(oc);
	} });

	commandMap.insert({ CommunicationKeys::BombUpdate, [this](sf::Packet packet) {//actualizem una bomba que ens ha dit el servidor
		OnlineBomb ob;
		packet >> ob;

		std::mutex myMutex;
		std::unique_lock<std::mutex> lock(myMutex);
		gameInit.wait(lock, [this] {return gameInitialized.load(); }); //Esperem a que s'hagi creat la partida

		game->HandleBombUpdate(ob);
	} });

	commandMap.insert({ CommunicationKeys::BombExplode, [this](sf::Packet packet) { //Comencem la explosio d'una bomba que ens ha dit el servidor EX:11 EX:12
		int bombId;
		packet >> bombId;

		game->ExplodeBomb(bombId);
	} });

	commandMap.insert({ CommunicationKeys::GameElements, [this](sf::Packet packet) { //carreguem tots els elements de la partida
		CPVector<OnlineClient> oc;
		CPVector<OnlineBomb> ob;
		CPVector<ChatMessage> cm;
		packet >> oc >> ob >> cm;

		std::mutex myMutex;
		std::unique_lock<std::mutex> lock(myMutex);
		gameInit.wait(lock, [this] {return gameInitialized.load(); }); //Esperem a que s'hagi creat la partida
		game->HandleLoadEntities(oc);
		game->HandleLoadBombs(ob);
		game->LoadMessageHistory(cm);
	}});

	commandMap.insert({ CommunicationKeys::Disconnect, [this](sf::Packet packet) { //Desconcetem el player que toqui, amb animacio de mort si es pq lhan matat //EX:4 //EX:5 EX:5.1
		int idLeft;
		bool killed;
		packet >> idLeft >> killed;

		if (!killed)
			game->EntityLeft(idLeft);
		else
			game->EntityDead(idLeft);		

	}});

	commandMap.insert({ CommunicationKeys::Dead, [this](sf::Packet packet) { //Si maten el nostre player fa animacio de mort i al acabar tanquem la sessio EX:5 EX:5.1
		player->SetLives(0);
		player->dead = true;
		Animation* a = game->GetPlayer()->GetAnim();
		if (a != nullptr)
			a->Stop();
		a = game->GetPlayer()->GetLink()->deadAnim;
		a->PlayOnce([this] {sessionEnd = true; });
	}});

	commandMap.insert({ CommunicationKeys::Hit, [this](sf::Packet packet) { //Si el sever ens diu que el nostre player ha rebut un atac cridem la funcio que sencarrega
		sf::Vector2f newPos;
		int newLives;
		packet >> newPos.x >> newPos.y >> newLives;
		game->GetPlayer()->HitReceived(newPos, newLives);
	} });	
}

Client::~Client()
{
	player = nullptr;

	delete game;
	game = nullptr;

	delete udpSocket;
	udpSocket = nullptr;

	delete threadReceive;
	threadReceive = nullptr;
}

void Client::ConnectToServer() //EX:1
{
	std::cout << "Enter your nickname: ";
	std::cin >> name;

	threadReceive = new std::thread(&Client::HandleClientReceiveData, this);
	threadReceive->detach();
	sf::Packet p;
	p << static_cast<int>(CommunicationKeys::Login) << name; //enviem el server que ens volem connectar i el nostre nom
	SendData(p);

	HandleGameInterface(); //Creacio i bucle del joc
}

void Client::HandleClientReceiveData() //Bucl eper rebre info del server
{
	sf::Packet* p = new sf::Packet();
	sf::IpAddress* ipAdr = new sf::IpAddress();
	unsigned short* port = new unsigned short;

	while (true) {

		if (udpSocket->receive(*p, *ipAdr, *port) == sf::Socket::Done)
		{
			int keyInt;
			*p >> keyInt;
			CommunicationKeys key = static_cast<CommunicationKeys>(keyInt);
			ExecuteCommand(key, *p);
		}
	}
}

void Client::HandleGameInterface()
{	
	std::unique_lock<std::mutex> lock(clientLoggedMutex);
	clientLogged.wait(lock, [this] {return id.load() != -1; });	

	//Creem la partida
	game = new Game(id, name, &sessionEnd, connectionTime, gameDuration, player, [this]() { sf::Packet pDisc; pDisc << static_cast<int>(CommunicationKeys::Disconnect) << id; return SendData(pDisc); }, [this](sf::Packet p) {return SendData(p); });
	gameInitialized.store(true);
	gameInit.notify_all(); //Avisem que s'ha creat la partida

	game->Update();
}

bool Client::SendData(sf::Packet p) //Envia info al server
{
	if (udpSocket->send(p, "127.0.0.1", 5000) == sf::Socket::Done) 
	{		
		//std::cout << "Client sent message complete" << std::endl;
		return true;
	}
	//std::cout << "Client sent message failed" << std::endl;
	return false;
}

void Client::ExecuteCommand(CommunicationKeys key, sf::Packet data)
{
	auto it = commandMap.find(key);
	if (it != commandMap.end())
		it->second(data);
}

Game::Game(int _idClient, std::string _name, std::atomic<bool>* _sessionEnd, sf::Uint32 _time, sf::Uint32 _gameDuration, Player* _player, std::function<bool()> _OnDisconnect, std::function<bool(sf::Packet)> _SendDataFunc)
{
	OnDisconnect = _OnDisconnect;
	SendDataFunc = _SendDataFunc;

	idClient = _idClient;
	name = _name;
	gameDuration = _gameDuration;
	connectionTime = _time;
	sessionEnd = _sessionEnd;
	gameEnd.store(false);
	backgroundRect = new sf::RectangleShape(sf::Vector2f(700, 500));
	backgroundRect->setOrigin(sf::Vector2f(350, 250));
	backgroundRect->setPosition(sf::Vector2f(400, 300));
	backgroundRect->setFillColor(sf::Color(100, 100, 140));
	points.store(0);
	font.loadFromFile("ResourcesClass/Minecraft.ttf");

	pointsText.setFont(font);	
	pointsText.setCharacterSize(20);	
	pointsText.setFillColor(sf::Color::White);

	timeRemainingText.setFont(font);
	timeRemainingText.setCharacterSize(20);
	timeRemainingText.setFillColor(sf::Color::White);

	resultsText.setFont(font);
	resultsText.setFillColor(sf::Color::White);

	backgroundChatRect = new sf::RectangleShape(sf::Vector2f(200, 400));
	backgroundChatRect->setPosition(sf::Vector2f(600, 100));
	backgroundChatRect->setFillColor(sf::Color(40, 40, 60));

	backgroundChatDepthRect = new sf::RectangleShape(sf::Vector2f(210, 410));
	backgroundChatDepthRect->setPosition(sf::Vector2f(595, 95));
	backgroundChatDepthRect->setFillColor(sf::Color(200, 200, 200));

	backgroundInputRect = new sf::RectangleShape(sf::Vector2f(200, 30));
	backgroundInputRect->setPosition(sf::Vector2f(600, 470));
	backgroundInputRect->setFillColor(sf::Color(80, 120, 130));

	inputText.setFont(font); // Establecer la fuente
	inputText.setCharacterSize(14); // Establecer el tamaño del texto
	inputText.setFillColor(sf::Color::White); // Establecer el color del texto
	inputText.setString("Enter text...");
	inputText.setPosition(605, 480); //GAMEWIDTH + 15, TOTALHEIGTH - 35

	mssgText.setFont(font);
	mssgText.setCharacterSize(14);

	player = _player;
	gameWindow = new sf::RenderWindow(sf::VideoMode(800, 600), "Game Window"); //

}

Game::~Game()
{
	delete player;
	player = nullptr;

	playersMutex.lock();
	for (auto it = playersInGame.begin(); it != playersInGame.end(); it++) {
		delete it->second;
		it->second = nullptr;
	}
	playersInGame.clear();
	playersMutex.unlock();

	bombsMutex.lock();
	for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) {
		delete it->second;
		it->second = nullptr;
	}
	bombsInGame.clear();
	bombsMutex.unlock();

	gameMssgsMutex.lock();
	for (auto it = gameMessages.begin(); it != gameMessages.end(); it++) {
		delete (*it);
		(*it) = nullptr;
	}
	gameMessages.clear();
	gameMssgsMutex.unlock();

	delete gameWindow;
	gameWindow = nullptr;

	delete sessionEnd;
	sessionEnd = nullptr;

	delete backgroundChatDepthRect;
	backgroundChatDepthRect = nullptr;
	delete backgroundChatRect;
	backgroundChatRect = nullptr;
	delete backgroundInputRect;
	backgroundInputRect = nullptr;
	delete backgroundRect;
	backgroundRect = nullptr;
}

void Game::Update()
{
	sf::Clock clock;
	lastElapsedTime = elapsedTime;
	while (!sessionEnd->load()) {
		elapsedTime = connectionTime + clock.getElapsedTime().asMilliseconds();
		//std::cout << "Elapsed is: " << elapsedTime << std::endl;
		sf::Uint16 deltaTime = elapsedTime - lastElapsedTime;

		HandleTimeCounter(deltaTime);
		HandleWindow(deltaTime); //Controla els inputs
		HandleEntites(deltaTime); //Controla els players
		HandleBombs(deltaTime); //Controla les bombes

		Render(); //Pinta la pantalla

		lastElapsedTime = elapsedTime;
	}
}

void Game::HandleWindow(sf::Uint16 deltaTime)
{
	sf::Event event;
	while (gameWindow->pollEvent(event)) {
		if (event.type == sf::Event::Closed || close) {//EX:1
			// Si se cierra la ventana, terminar el bucle principal		
			if (close || OnDisconnect()) { //Si es dona a la creu avisem al server de q ens desconectem i tanquem sessio
				gameWindow->close();
				sessionEnd->store(true);
			}
		}
		else {
			if (gameMode) {
				if (event.type == sf::Event::EventType::KeyPressed || event.type == sf::Event::EventType::KeyReleased) { //Si el player te una accio asociada a la tecla q apretem la cridem
					auto it = player->keyEventMap.find(event.key.code);
					if (it != player->keyEventMap.end())
						it->second(event.type == sf::Event::KeyPressed, elapsedTime, deltaTime);
					else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {

						if (player->GetDir().x != 0 || player->GetDir().y != 0) {
							int frame = 0;
							switch (player->GetOrientation())
							{
							case UP:
								frame = 0;
								player->RegisterInput(false, static_cast<sf::Uint32>(sf::Keyboard::Key::Up), elapsedTime, elapsedTime - lastElapsedTime);
								break;
							case DOWN:
								frame = 6;
								player->RegisterInput(false, static_cast<sf::Uint32>(sf::Keyboard::Key::Down), elapsedTime, elapsedTime - lastElapsedTime);
								break;
							case LEFT:
								frame = 5;
								player->RegisterInput(false, static_cast<sf::Uint32>(sf::Keyboard::Key::Left), elapsedTime, elapsedTime - lastElapsedTime);
								break;
							case RIGHT:
								frame = 5;
								player->RegisterInput(false, static_cast<sf::Uint32>(sf::Keyboard::Key::Right), elapsedTime, elapsedTime - lastElapsedTime);
								break;
							default:
								break;
							}
							player->CancelMovement(frame);
						}
						
						gameMode = false;
					}
				}
			}
			else {
				if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter && !input.empty()) {

					ChatMessage* mssg = new ChatMessage(name, input, GetTime(), false);
					input.clear();
					inputText.setString("Enter text...");
					gameMssgsMutex.lock();
					gameMessages.push_back(mssg);
					gameMssgsMutex.unlock();

					sf::Packet p;
					p << static_cast<int>(CommunicationKeys::Message) << idClient << *mssg;
					SendDataFunc(p);
				}
				else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {
					gameMode = true;
				}
				else if (event.type == sf::Event::TextEntered) {
					if (event.text.unicode < 128 && event.text.unicode != 8 && event.text.unicode != 13 && event.text.unicode != 9 && inputText.getGlobalBounds().width < 100) { // Caracteres ASCII sense "backspace" i "enter" per escriure nom de la sala
						input += static_cast<char>(event.text.unicode);
						inputText.setString(input);
					}
					else if (event.text.unicode == 8 && !input.empty()) { // Tecla "backspace"
						input.pop_back();
						if (input.size() <= 0) 
							inputText.setString("Enter text...");						
						else
							inputText.setString(input);
					}
				}
			}
		}
		
	}
}

void Game::HandleEntites(sf::Uint16 deltaTime)
{
	playersRemoveMutex.lock();
	for (auto it = playersPendingRemove.begin(); it != playersPendingRemove.end(); it++) { //Si un player ha acabat la seva animacio de mort el borrem
		if (elapsedTime - it->second >= 1000) { 
			EntityLeft(it->first);
		}
	}
	playersRemoveMutex.unlock();
	std::unique_lock<std::mutex> lock(playersMutex);	
	for (auto it = playersInGame.begin(); it != playersInGame.end(); it++) { //Revisem posicions i animacions

		it->second->playerCharacter->lifesNum = it->second->playerClient->GetLives();

		if (it->second->playerClient->GetAttacking().first) {
			if (elapsedTime - it->second->playerClient->GetAttacking().second >= 500) //Si un player esta fent la animacio d'atacar no cal revisar res mes, si ya la ha acabat seguim
				it->second->playerClient->SetAttacking({ false, 0 });
			else
				continue;
		}

		if (it->second->playerClient->GetBombAnimation().first) {
			if (elapsedTime - it->second->playerClient->GetBombAnimation().second >= 500) {//Si un player esta fent la animacio d'agafar o tirar no cal revisar res mes, si ya la ha acabat seguim
				it->second->playerClient->SetBombAnimation({ false, 0 });
			}
			else
				continue;
		}		

		float velMagnitudeScaled = deltaTime * 0.1f;
		sf::Vector2f velVector = ((sf::Vector2f)it->second->playerClient->GetDir() * velMagnitudeScaled);
		sf::Vector2f newPos = it->second->playerClient->GetPos() + velVector; //la nova posicio es la actual mes la direccio en la que s'esta moven (si es mou) per la velocitat de moviment pel temps que ha passat desde lultim moviment (deltaTime)

		if (newPos.x < 50)
			newPos.x = 50;
		if (newPos.x > 720)
			newPos.x = 720;
		if (newPos.y < 50)
			newPos.y = 50;
		if (newPos.y > 520)
			newPos.y = 520;


		if (it->second->playerCurrentAnimation != nullptr && it->second->playerClient->GetDir().x == 0 && it->second->playerClient->GetDir().y == 0) { //Si el player esta fent una animacio i ya ha acabat o esta quiet la parem a un frame que quedi be
			int frame = 0;
			it->second->playerCurrentAnimation->Stop();
			switch (it->second->playerClient->orientation)
			{
			case UP:
				if(!it->second->playerClient->hasBomb)
					it->second->playerCurrentAnimation = it->second->playerCharacter->moveUpAnim;
				else
					it->second->playerCurrentAnimation = it->second->playerCharacter->chargeUpAnim;
				frame = 0;
				break;
			case DOWN:
				if (!it->second->playerClient->hasBomb)
					it->second->playerCurrentAnimation = it->second->playerCharacter->moveDownAnim;
				else
					it->second->playerCurrentAnimation = it->second->playerCharacter->chargeDownAnim;
				frame = 6;
				break;
			case LEFT:
				if (!it->second->playerClient->hasBomb)
					it->second->playerCurrentAnimation = it->second->playerCharacter->moveLeftAnim;
				else
					it->second->playerCurrentAnimation = it->second->playerCharacter->chargeLeftAnim;
				frame = 5;
				break;
			case RIGHT:
				if (!it->second->playerClient->hasBomb)
					it->second->playerCurrentAnimation = it->second->playerCharacter->moveRightAnim;
				else
					it->second->playerCurrentAnimation = it->second->playerCharacter->chargeRightAnim;
				frame = 5;
				break;
			default:
				break;
			}
			it->second->playerCurrentAnimation->_currentFrame = frame;
			it->second->playerCurrentAnimation->SetCurrentFrame();
			it->second->playerCurrentAnimation = nullptr;
		}

		it->second->playerClient->SetPos(newPos); //Actualitzem la nova posicio del player
		it->second->playerCharacter->setPosition(newPos); //Tambe actualitzem la posicio del sprite
	}
	lock.unlock();
	player->Update(deltaTime); //Revisem el nostre player
}

void Game::HandleBombs(sf::Uint16 deltaTime)
{
	std::unique_lock<std::mutex> lock(bombsMutex);
	for (auto it = bombsInCountown.begin(); it != bombsInCountown.end(); it++) { //Si una bomba que estava explotant ya ha acabat d'explotar la borrem
		if (elapsedTime - it->second >= 1000)
			bombsInGame.erase(it->first);
	}
	for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) { //Actualitzem la posicio de les bombes que tenen propietari a la mateixa que aquest + un offset
		int idPlayer = it->second->GetOwnerId();
		if (idPlayer != -1) {
			playersMutex.lock();
			auto it2 = playersInGame.find(idPlayer);
			if (it2 != playersInGame.end()) { //Si el propietari es el player d'un altre client
				it->second->SetCurrentPos(it2->second->playerClient->GetPos(), true); 
			}
			else {
				if (idClient == idPlayer) { //Si el propietari es el nostre player
					it->second->SetCurrentPos(player->GetPos(), true);
				}
			}
			playersMutex.unlock();
		}
	}
	lock.unlock();
}

void Game::HandleTimeCounter(sf::Uint16 deltaTime)
{
	sf::Uint32 time = gameDuration - elapsedTime;

	sf::Uint32 totalSeconds = time / 1000;
	sf::Uint32 minutes = totalSeconds / 60;        
	sf::Uint32 seconds = totalSeconds % 60;        

	timeRemainingText.setString("Time Remaining: " + std::to_string(minutes) + " min " + std::to_string(seconds) + " s");
	timeRemainingText.setPosition(sf::Vector2f(50, 15));
	pointsText.setString("Points: " + std::to_string(points));
	pointsText.setPosition(sf::Vector2f(750 - pointsText.getGlobalBounds().width, 15));
}

sf::Uint32 Game::GetTime()
{	
	// Obtenim la data i hora actual
	std::time_t now = std::time(nullptr);
	std::tm* local_time = std::localtime(&now);
	// Construir la data i hora com una cadena de caracters
	char datetime_str[20];
	std::strftime(datetime_str, sizeof(datetime_str), "%Y%m%d%H%M%S", local_time);
	// Convertir la cadena de caracters a un sf::Uint64
	sf::Uint64 timestamp;
	std::sscanf(datetime_str, "%llu", &timestamp);
	timestamp %= 1000000;
	return timestamp % 1000000;
}

std::string Game::GetMssgTime(sf::Uint32 timeStamp)
{
	std::string date = std::to_string(timeStamp); //transforma el valor de timestamp a una data normal

	int zeros = 6 - date.size();
	std::string extra;
	for (int i = 0; i < zeros; i++) {
		extra += "0";
	}
	extra += date;
	std::string hour = extra.substr(0, 2);
	std::string min = extra.substr(2, 2);
	std::string sec = extra.substr(4, 2);
	std::string dateFixed = hour + ':' + min + "| ";

	return dateFixed;
}


void Game::Render()
{
	gameWindow->clear();

	if (!gameEnd.load()) {
		gameWindow->draw(*backgroundRect); //Es pinta el fondo
		std::unique_lock<std::mutex> lock2(bombsMutex);
		for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) { //Es pinten les bombes
			gameWindow->draw(*(it->second->bc));
		}
		lock2.unlock();
		player->DrawPlayer(gameWindow);
		std::unique_lock<std::mutex> lock(playersMutex);
		for (auto it = playersInGame.begin(); it != playersInGame.end(); it++) { //Es pinten els players
			it->second->playerCharacter->Draw(gameWindow);
			if (it->second->playerClient->GetAttacking().first) {
				gameWindow->draw(it->second->playerCharacter->sword);
			}
		}
		lock.unlock();

		if (!gameMode) {
			gameWindow->draw(*backgroundChatDepthRect);
			gameWindow->draw(*backgroundChatRect);
			gameWindow->draw(*backgroundInputRect);
			gameWindow->draw(inputText);
			gameMssgsMutex.lock();
			int ind = 0; //450 dif de 30
			for (auto it = gameMessages.rbegin(); it != gameMessages.rend(); it++, ind++) {
				if (ind > 15)
					break;

				mssgText.setPosition(sf::Vector2f(605, 447 - (23 * ind)));
				mssgText.setString(GetMssgTime((*it)->timeStamp) + (*it)->clientName + ": " + (*it)->message);
				mssgText.setFillColor((*it)->isLog ? sf::Color(120, 120, 120) : sf::Color(200, 200, 200));
				gameWindow->draw(mssgText);
			}
			gameMssgsMutex.unlock();
		}

		gameWindow->draw(pointsText);
		gameWindow->draw(timeRemainingText);
	}	
	else {
		int ind = 1;
		resultsText.setCharacterSize(30);
		resultsText.setString("GAME OVER");
		resultsText.setPosition(sf::Vector2f(310, 40));
		gameWindow->draw(resultsText);
		resultsText.setCharacterSize(16);

		for (auto it = gameResults.rbegin(); it != gameResults.rend(); ++it) {
			if (ind > 13)
				break;

			resultsText.setString(std::to_string(ind) + " - " + it->second + " => " + std::to_string(it->first) + " pts");
			resultsText.setPosition(sf::Vector2f(345 , 100 + ((ind - 1) * 40)));
			gameWindow->draw(resultsText);

			ind++;
		}
	}

	gameWindow->display();
}

Player* Game::GetPlayer() const
{
	if(player != nullptr)
		return player; 
	return nullptr;
}

//Si el client ya existia s'actualitza la seva posicio i animacio segons calgui, sino es crea i se li fica la posicio i animacio que toqui
void Game::HandleEntityUpdate(OnlineClient oc)
{
	std::unique_lock<std::mutex> lock(playersMutex);
	std::map<int, OnlinePlayer*>::iterator it = playersInGame.find(oc.GetId());
	if (it != playersInGame.end()) {
		*it->second->playerClient = oc;
		it->second->playerCharacter->setPosition(oc.GetPos());
		switch (it->second->playerClient->orientation)
		{
		case Orientation::UP:
			if (it->second->playerClient->GetAttacking().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->atackUpAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					it->second->playerCurrentAnimation = it->second->playerCharacter->atackUpAnim;
					it->second->playerCharacter->sword.setRotation(0);
					it->second->playerCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetUp);
					it->second->playerCurrentAnimation->PlayOnce([this]() {
					});
				}
				break;
			}

			if (it->second->playerClient->GetBombAnimation().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->grabUpAnim && it->second->playerCurrentAnimation != it->second->playerCharacter->throwUpAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					if(it->second->playerClient->hasBomb)
						it->second->playerCurrentAnimation = it->second->playerCharacter->throwUpAnim;
					else
						it->second->playerCurrentAnimation = it->second->playerCharacter->grabUpAnim;					
					it->second->playerCurrentAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (it->second->playerCurrentAnimation == it->second->playerCharacter->moveUpAnim || it->second->playerCurrentAnimation == it->second->playerCharacter->chargeUpAnim)
				break;
			else if (it->second->playerCurrentAnimation != nullptr)
				it->second->playerCurrentAnimation->Stop();

			if(!it->second->playerClient->hasBomb)
				it->second->playerCurrentAnimation = it->second->playerCharacter->moveUpAnim;
			else
				it->second->playerCurrentAnimation = it->second->playerCharacter->chargeUpAnim;

			if (it->second->playerClient->GetDir().x == 0 && it->second->playerClient->GetDir().y == 0) {
				it->second->playerCurrentAnimation->_currentFrame = 0;
				it->second->playerCurrentAnimation->SetCurrentFrame();
				it->second->playerCurrentAnimation = nullptr;
			}
			else
				it->second->playerCurrentAnimation->Play(true, true);
			break;
		case Orientation::DOWN:
			if (it->second->playerClient->GetAttacking().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->atackDownAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					it->second->playerCurrentAnimation = it->second->playerCharacter->atackDownAnim;
					it->second->playerCharacter->sword.setRotation(180);
					it->second->playerCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetDown);
					it->second->playerCurrentAnimation->PlayOnce([this]() {
					});
				}
				break;
			}

			if (it->second->playerClient->GetBombAnimation().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->grabDownAnim && it->second->playerCurrentAnimation != it->second->playerCharacter->throwDownAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					if (it->second->playerClient->hasBomb)
						it->second->playerCurrentAnimation = it->second->playerCharacter->throwDownAnim;
					else
						it->second->playerCurrentAnimation = it->second->playerCharacter->grabDownAnim;
					it->second->playerCurrentAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (it->second->playerCurrentAnimation == it->second->playerCharacter->moveDownAnim || it->second->playerCurrentAnimation == it->second->playerCharacter->chargeDownAnim)
				break;
			else if (it->second->playerCurrentAnimation != nullptr)
				it->second->playerCurrentAnimation->Stop();

			if(!it->second->playerClient->hasBomb)
				it->second->playerCurrentAnimation = it->second->playerCharacter->moveDownAnim;
			else
				it->second->playerCurrentAnimation = it->second->playerCharacter->chargeDownAnim;

			if (it->second->playerClient->GetDir().x == 0 && it->second->playerClient->GetDir().y == 0) {
				it->second->playerCurrentAnimation->_currentFrame = 6;
				it->second->playerCurrentAnimation->SetCurrentFrame();
				it->second->playerCurrentAnimation = nullptr;
			}
			else
				it->second->playerCurrentAnimation->Play(true, true);
			break;
		case Orientation::LEFT:
			if (it->second->playerClient->GetAttacking().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->atackLeftAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					it->second->playerCurrentAnimation = it->second->playerCharacter->atackLeftAnim;
					it->second->playerCharacter->sword.setRotation(-90);
					it->second->playerCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetLeft);
					it->second->playerCurrentAnimation->PlayOnce([this]() {
					});
				}
				break;
			}

			if (it->second->playerClient->GetBombAnimation().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->grabLeftAnim && it->second->playerCurrentAnimation != it->second->playerCharacter->throwLeftAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					if (it->second->playerClient->hasBomb)
						it->second->playerCurrentAnimation = it->second->playerCharacter->throwLeftAnim;
					else
						it->second->playerCurrentAnimation = it->second->playerCharacter->grabLeftAnim;
					it->second->playerCurrentAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (it->second->playerCurrentAnimation == it->second->playerCharacter->moveLeftAnim || it->second->playerCurrentAnimation == it->second->playerCharacter->chargeLeftAnim)
				break;
			else if (it->second->playerCurrentAnimation != nullptr)
				it->second->playerCurrentAnimation->Stop();

			if(!it->second->playerClient->hasBomb)
				it->second->playerCurrentAnimation = it->second->playerCharacter->moveLeftAnim;
			else
				it->second->playerCurrentAnimation = it->second->playerCharacter->chargeLeftAnim;

			if (it->second->playerClient->GetDir().x == 0 && it->second->playerClient->GetDir().y == 0) {
				it->second->playerCurrentAnimation->_currentFrame = 5;
				it->second->playerCurrentAnimation->SetCurrentFrame();
				it->second->playerCurrentAnimation = nullptr;
			}
			else
				it->second->playerCurrentAnimation->Play(false, true);
			break;
		case Orientation::RIGHT:

			if (it->second->playerClient->GetAttacking().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->atackRightAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					it->second->playerCurrentAnimation = it->second->playerCharacter->atackRightAnim;
					it->second->playerCharacter->sword.setRotation(90);
					it->second->playerCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetRight);
					it->second->playerCurrentAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (it->second->playerClient->GetBombAnimation().first) {
				if (it->second->playerCurrentAnimation != it->second->playerCharacter->grabRightAnim && it->second->playerCurrentAnimation != it->second->playerCharacter->throwRightAnim) {
					it->second->playerClient->SetDir(sf::Vector2i(0, 0));
					if (it->second->playerCurrentAnimation != nullptr)
						it->second->playerCurrentAnimation->Stop();
					if (it->second->playerClient->hasBomb)
						it->second->playerCurrentAnimation = it->second->playerCharacter->throwRightAnim;
					else
						it->second->playerCurrentAnimation = it->second->playerCharacter->grabRightAnim;
					it->second->playerCurrentAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (it->second->playerCurrentAnimation == it->second->playerCharacter->moveRightAnim || it->second->playerCurrentAnimation == it->second->playerCharacter->chargeRightAnim)
				break;
			else if (it->second->playerCurrentAnimation != nullptr)
				it->second->playerCurrentAnimation->Stop();

			if(!it->second->playerClient->hasBomb)
				it->second->playerCurrentAnimation = it->second->playerCharacter->moveRightAnim;
			else
				it->second->playerCurrentAnimation = it->second->playerCharacter->chargeRightAnim;

			if (it->second->playerClient->GetDir().x == 0 && it->second->playerClient->GetDir().y == 0) {
				it->second->playerCurrentAnimation->_currentFrame = 5;
				it->second->playerCurrentAnimation->SetCurrentFrame();
				it->second->playerCurrentAnimation = nullptr;
			}
			else
				it->second->playerCurrentAnimation->Play(false, true);
			break;
		default:
			break;
		}
	}
	else {
		OnlineClient* newOnlineClient = new OnlineClient(oc);
		Animation* newAnimation = new Animation();
		LinkCharacter* newLinkCharacter = new LinkCharacter(sf::Vector2u(oc.GetPos().x, oc.GetPos().y), oc.GetName(), sf::Color(255, 0, 0, 120)); //EX:3
		playersInGame.insert(std::make_pair(oc.GetId(), new OnlinePlayer(newOnlineClient,newAnimation, newLinkCharacter)));
		switch (newOnlineClient->orientation)
		{
		case Orientation::UP:
			if (newOnlineClient->GetAttacking().first) {
				if (newAnimation != newLinkCharacter->atackUpAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					newAnimation = newLinkCharacter->atackUpAnim;
					newLinkCharacter->sword.setRotation(0);
					newLinkCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetUp);
					newAnimation->PlayOnce([this]() {

					});
				}
				break;
			}

			if (newOnlineClient->GetBombAnimation().first) {
				if (newAnimation != newLinkCharacter->grabUpAnim && newAnimation != newLinkCharacter->throwUpAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					if(newOnlineClient->hasBomb)
						newAnimation = newLinkCharacter->throwUpAnim;
					else
						newAnimation = newLinkCharacter->grabUpAnim;					
					newAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if(!newOnlineClient->hasBomb)
				newAnimation = newLinkCharacter->moveUpAnim;
			else
				newAnimation = newLinkCharacter->chargeUpAnim;

			if (newOnlineClient->GetDir().x == 0 && newOnlineClient->GetDir().y == 0) {
				newAnimation->_currentFrame = 0;
				newAnimation->SetCurrentFrame();
				newAnimation = nullptr;
			}
			else
				newAnimation->Play(true, true);
			break;
		case Orientation::DOWN:
			if (newOnlineClient->GetAttacking().first) {
				if (newAnimation != newLinkCharacter->atackDownAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					newAnimation = newLinkCharacter->atackDownAnim;
					newLinkCharacter->sword.setRotation(180);
					newLinkCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetDown);
					newAnimation->PlayOnce([this]() {

						});
				}
				break;
			}

			if (newOnlineClient->GetBombAnimation().first) {
				if (newAnimation != newLinkCharacter->grabDownAnim && newAnimation != newLinkCharacter->throwDownAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					if (newOnlineClient->hasBomb)
						newAnimation = newLinkCharacter->throwDownAnim;
					else
						newAnimation = newLinkCharacter->grabDownAnim;
					newAnimation->PlayOnce([this]() {});
				}
				break;
			}

			if (!newOnlineClient->hasBomb)
				newAnimation = newLinkCharacter->moveDownAnim;
			else
				newAnimation = newLinkCharacter->chargeDownAnim;

			if (newOnlineClient->GetDir().x == 0 && newOnlineClient->GetDir().y == 0) {
				newAnimation->_currentFrame = 6;
				newAnimation->SetCurrentFrame();
				newAnimation = nullptr;
			}
			else
				newAnimation->Play(true, true);
			break;
		case Orientation::LEFT:
			if (newOnlineClient->GetAttacking().first) {
				if (newAnimation != newLinkCharacter->atackLeftAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					newAnimation = newLinkCharacter->atackLeftAnim;
					newLinkCharacter->sword.setRotation(-90);
					newLinkCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetLeft);
					newAnimation->PlayOnce([this]() {

					});
				}
				break;
			}

			if (newOnlineClient->GetBombAnimation().first) {
				if (newAnimation != newLinkCharacter->grabLeftAnim && newAnimation != newLinkCharacter->throwLeftAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					if (newOnlineClient->hasBomb)
						newAnimation = newLinkCharacter->throwLeftAnim;
					else
						newAnimation = newLinkCharacter->grabLeftAnim;
					newAnimation->PlayOnce([this]() {});
				}
				break;
			}
			if (!newOnlineClient->hasBomb)
				newAnimation = newLinkCharacter->moveLeftAnim;
			else
				newAnimation = newLinkCharacter->chargeLeftAnim;

			if (newOnlineClient->GetDir().x == 0 && newOnlineClient->GetDir().y == 0) {
				newAnimation->_currentFrame = 5;
				newAnimation->SetCurrentFrame();
				newAnimation = nullptr;
			}
			else
				newAnimation->Play(false, true);
			break;
		case Orientation::RIGHT:
			if (newOnlineClient->GetAttacking().first) {
				if (newAnimation != newLinkCharacter->atackRightAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					newAnimation = newLinkCharacter->atackRightAnim;
					newLinkCharacter->sword.setRotation(90);
					newLinkCharacter->sword.setPosition(it->second->playerCharacter->getPosition() + it->second->playerCharacter->swordOffsetRight);
					newAnimation->PlayOnce([this]() {

						});
				}
				break;
			}

			if (newOnlineClient->GetBombAnimation().first) {
				if (newAnimation != newLinkCharacter->grabRightAnim && newAnimation != newLinkCharacter->throwRightAnim) {
					newOnlineClient->SetDir(sf::Vector2i(0, 0));
					if (newAnimation != nullptr)
						newAnimation->Stop();
					if (newOnlineClient->hasBomb)
						newAnimation = newLinkCharacter->throwRightAnim;
					else
						newAnimation = newLinkCharacter->grabRightAnim;
					newAnimation->PlayOnce([this]() {});
				}
				break;
			}
			if (!newOnlineClient->hasBomb)
				newAnimation = newLinkCharacter->moveRightAnim;
			else
				newAnimation = newLinkCharacter->chargeRightAnim;

			if (newOnlineClient->GetDir().x == 0 && newOnlineClient->GetDir().y == 0) {
				newAnimation->_currentFrame = 5;
				newAnimation->SetCurrentFrame();
				newAnimation = nullptr;
			}
			else
				newAnimation->Play(false, true);
			break;
		default:
			break;
		}		
	}
	lock.unlock();
}

void Game::HandleBombUpdate(OnlineBomb ob)
{
	bombsMutex.lock();
	auto it = bombsInGame.find(ob.GetBombId());
	if (it == bombsInGame.end()) { //Si la bomba encara no estava al joc es crea i se li crea el sprite
		bombsInGame.insert({ ob.GetBombId(), new OnlineBomb(ob) });
		bombsInGame[ob.GetBombId()]->CreateSprite();
	}
	else {
		bombsInGame[ob.GetBombId()]->SetOwnerId(ob.GetOwnerId()); //actualitzem propietari
		bombsInGame[ob.GetBombId()]->SetThrewTime(ob.GetThrewTime()); //actualitzem el temops de quan sha llençat
		bombsInGame[ob.GetBombId()]->SetCurrentPos(ob.GetCurrentPos(), false); //actualitzem pos
		if (ob.GetOwnerId() != -1) { //Si te algun propietari		
			std::unique_lock<std::mutex> lock(playersMutex);
			auto it2 = playersInGame.find(ob.GetOwnerId());
			if (it2 != playersInGame.end()) { //Si la bomba es dun player dun altre client
				if (ob.GetThrewTime() == 0) { //Si encara no s'ha llençat marquem a aquell player com q porta la bomba, i aprofitem per corregir posicio i color d'aquesta
					bombsInGame[ob.GetBombId()]->SetCurrentPos(it2->second->playerClient->GetPos(), true);
					it2->second->playerClient->hasBomb = true;
					bombsInGame[ob.GetBombId()]->bc->setColor(sf::Color(255, 0, 0, 120));
				}
				else {
					it2->second->playerClient->hasBomb = false; //Si ya sha llençat marquem a aquell player com que ya no porta bomba
				}
			}
			else if (idClient == ob.GetOwnerId()) {//Si la bomba es del nostre player
				if (ob.GetThrewTime() == 0) {  //Si encara no s'ha llençat marquem el nostre player com q porta la bomba, i aprofitem per corregir posicio i color d'aquesta
					bombsInGame[ob.GetBombId()]->SetCurrentPos(player->GetPos(), true); 
					player->hasBomb = true;
					bombsInGame[ob.GetBombId()]->bc->setColor(sf::Color::Green);
				}
				else {
					player->hasBomb = false; //Si ya sha llençat marquem el nostre player com que ya no porta bomba
				}
			}
		}		
		
	}
	bombsMutex.unlock();
}

void Game::HandleLoadEntities(CPVector<OnlineClient> gameEntities) //Carregem tots els players amb la seva posicio i animacio
{
	std::unique_lock<std::mutex> lock(playersMutex);
	for (CPVector<OnlineClient>::iterator it = gameEntities.begin(); it != gameEntities.end(); it++) {
		OnlinePlayer* op = new OnlinePlayer((*it), new Animation(), new LinkCharacter(sf::Vector2u((*it)->GetPos().x, (*it)->GetPos().y), (*it)->GetName(), sf::Color(255, 0, 0, 120)));
		playersInGame.insert(std::make_pair((*it)->GetId(), op));

		int frame = 0;
		switch (op->playerClient->orientation)
		{
		case UP:
			if(!op->playerClient->hasBomb)
				op->playerCurrentAnimation = op->playerCharacter->moveUpAnim;
			else
				op->playerCurrentAnimation = op->playerCharacter->chargeUpAnim;
			frame = 0;
			break;
		case DOWN:
			if (!op->playerClient->hasBomb)
				op->playerCurrentAnimation = op->playerCharacter->moveDownAnim;
			else
				op->playerCurrentAnimation = op->playerCharacter->chargeDownAnim;
			frame = 6;
			break;
		case LEFT:
			if (!op->playerClient->hasBomb)
				op->playerCurrentAnimation = op->playerCharacter->moveLeftAnim;
			else
				op->playerCurrentAnimation = op->playerCharacter->chargeLeftAnim;
			frame = 5;
			break;
		case RIGHT:
			if (!op->playerClient->hasBomb)
				op->playerCurrentAnimation = op->playerCharacter->moveRightAnim;
			else
				op->playerCurrentAnimation = op->playerCharacter->chargeRightAnim;
			frame = 5;
			break;
		default:
			break;
		}
		op->playerCurrentAnimation->_currentFrame = frame;
		op->playerCurrentAnimation->SetCurrentFrame();
		op->playerCurrentAnimation = nullptr;
	}
	lock.unlock();	
}

void Game::HandleLoadBombs(CPVector<OnlineBomb> gameBombs) //Carregem totes les bombes amb el seu color i poscio
{
	std::unique_lock<std::mutex> lock(bombsMutex);
	for (CPVector<OnlineBomb>::iterator it = gameBombs.begin(); it != gameBombs.end(); it++) {
		bombsInGame.insert(std::make_pair((*it)->GetBombId(), (*it)));
		(*it)->CreateSprite();
		if((*it)->GetOwnerId() != -1 || (*it)->GetLastOwnerId() != -1)
			(*it)->bc->setColor(sf::Color(255, 0, 0, 120));
	}
	lock.unlock();
}

void Game::EntityLeft(int idLeft) //Borrem la info i el player de la partdia del jugador q ha marxat //EX:4 //EX:5
{

	std::unique_lock<std::mutex> lock(playersMutex);
	if (playersInGame.find(idLeft) == playersInGame.end()){
		lock.unlock();
		return;
	}

	if (playersInGame[idLeft]->playerCharacter != nullptr) {
		delete playersInGame[idLeft]->playerCharacter;
		playersInGame[idLeft]->playerCharacter = nullptr;
	}
	if (playersInGame[idLeft]->playerClient != nullptr) {
		delete playersInGame[idLeft]->playerClient;
		playersInGame[idLeft]->playerClient = nullptr;
	}
	if (playersInGame[idLeft]->playerCurrentAnimation != nullptr) {
		delete playersInGame[idLeft]->playerCurrentAnimation;
		playersInGame[idLeft]->playerCurrentAnimation = nullptr;
	}	
	playersInGame.erase(idLeft);
	lock.unlock();
}

void Game::EntityDead(int idLeft) //Fem la animacio de mort del player que toqui
{
	playersMutex.lock();
	auto it = playersInGame.find(idLeft);
	if (it != playersInGame.end()) {
		playersMutex.unlock();
		it->second->playerClient->SetLives(0);
		it->second->playerCharacter->SetLifes(0);		
		playersRemoveMutex.lock();
		playersPendingRemove.insert({ it->first, elapsedTime });
		playersRemoveMutex.unlock();

		Animation* a = it->second->playerCurrentAnimation;
		if (a != nullptr)
			a->Stop();
		a = it->second->playerCharacter->deadAnim;
		a->PlayOnce([] {});
	}
	else
		playersMutex.unlock();

	
}

void Game::ExplodeBomb(int id) //Fem la animacio de explosio de la bomba que toqui EX:11 EX:12
{
	bombsMutex.lock();
	auto it = bombsInGame.find(id);
	if (it != bombsInGame.end()) {
		bombsInCountown.push_back({ id, elapsedTime });
		it->second->bc->currentAnim = it->second->bc->explosion;
		it->second->bc->currentAnim->PlayOnce([] {});
	}
	//bombsInGame.erase(id);
	bombsMutex.unlock();
}

void Game::PointsGained(int pointsToAdd)
{
	int lastPoints = points.load();
	points.store(lastPoints + pointsToAdd);
}

void Game::LoadMessage(ChatMessage mssg)
{
	gameMssgsMutex.lock();
	gameMessages.push_back(new ChatMessage(mssg));
	gameMssgsMutex.unlock();
}

void Game::LoadMessageHistory(CPVector<ChatMessage> mssgs)
{
	gameMssgsMutex.lock();
	for (auto it = mssgs.rbegin(); it != mssgs.rend(); it++) {
		gameMessages.push_back((*it));
	}
	gameMssgsMutex.unlock();
}

void Game::SetResults(CPVector<GamePoints> results)
{
	resultsMutex.lock();
	for (auto it = results.begin(); it != results.end(); it++) {
		gameResults.insert({(*it)->points, (*it)->name});
	}
	resultsMutex.unlock();

	gameEnd.store(true);
}




