#include "Server.h"
#include <iostream>
#include <random>
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

Server::Server()
{
    playersIdMutex.lock();
    sesionEnd.store(false);
    playersID.store(0);
    playersIdMutex.unlock();
    timeRemaining = gameDurationInMs;

    bombsID.store(0);

    //Quan un player es conecta se li pasa la seva id, el temps actual de la partida, i la seva velocitat i posicio. 
    //Tambe se li envien tots els players i bombes actuals de la partida, i s'informa a tots els altres clients de que un de nou s'ha unit a la partida 
    commandMap.insert({ CommunicationKeys::Login, [this](sf::Packet packet, OnlineClient * client) {//EX:1
        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::Login) << client->GetId() << elapsedTime.load() << playersVelocity << client->GetPos().x << client->GetPos().y << gameDurationInMs;
        SendData(client->GetId(), p);

        CPVector<OnlineClient> allClientsPlaying;
        clientsPlayingMutex.lock();
        for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++) {
            if (it->first != client->GetId())
                allClientsPlaying.push_back(it->second);
        }
        clientsPlayingMutex.unlock();

        CPVector<OnlineBomb> allBombsInGame;
        bombsInGameMutex.lock();
        for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) {
            allBombsInGame.push_back(it->second);
        }
        bombsInGameMutex.unlock();

        messagesHistoryMutex.lock();
        CPVector<ChatMessage> allMsgs = messagesHistory;
        messagesHistoryMutex.unlock();

        sf::Packet p2;
        p2 << static_cast<int>(CommunicationKeys::GameElements) << allClientsPlaying << allBombsInGame << allMsgs;
        SendData(client->GetId(), p2);

        sf::Packet p3;
        p3 << static_cast<int>(CommunicationKeys::PlayerUpdate) << *client;
        SendDataAllButOne(client->GetId(), p3);
    }});

    //Al rebre tots els inputs s'ordenen per temps i es calcula la posicio actual.
    //Si la poisicio actual es incorrecta, se li envia al player la nova posicio
    //S'envia la posicio, direccio i orientacio actual a els altres players
    commandMap.insert({ CommunicationKeys::Inputs, [this](sf::Packet packet, OnlineClient* client) { //EX:6
        CPVector<InputData> data;
        packet >> data;
        std::map<int, InputData> orderedInputs;
        sf::Vector2i curDir;
        sf::Vector2f curPos;
        bool positionRecalculated = false;

        for (auto it = data.begin(); it != data.end(); it++)
            orderedInputs.insert(std::make_pair((*it)->idInput, *(*it)));

        for (auto it = orderedInputs.begin(); it != orderedInputs.end(); it++) {
            if (!(*it).second.pressed) {
                curDir = sf::Vector2i(0, 0);
                continue;
            }


            auto it2 = std::next(it);
            if (it2 != orderedInputs.end())
                if ((*it).second.keyCode != static_cast<sf::Uint32>(sf::Keyboard::Key::Space) &&
                    (*it).second.keyCode != static_cast<sf::Uint32>(sf::Keyboard::Key::E) && (*it).second.timeStamp == (*it2).second.timeStamp)
                    continue;

            switch (static_cast<sf::Keyboard::Key>((*it).second.keyCode))
            {
            case sf::Keyboard::Key::Up:
                curDir = sf::Vector2i(0, -1);
                client->orientation = Orientation::UP;
                break;
            case sf::Keyboard::Key::Down:
                curDir = sf::Vector2i(0, 1);
                client->orientation = Orientation::DOWN;
                break;
            case sf::Keyboard::Key::Left:
                curDir = sf::Vector2i(-1, 0);
                client->orientation = Orientation::LEFT;
                break;
            case sf::Keyboard::Key::Right:
                curDir = sf::Vector2i(1, 0);
                client->orientation = Orientation::RIGHT;
                break;
            case sf::Keyboard::Key::Space:
                curDir = sf::Vector2i(0, 0);
                client->SetAttacking(std::make_pair(true, it->second.timeStamp));
                break;
            case sf::Keyboard::Key::E: //EX:9
                curDir = sf::Vector2i(0, 0);
                client->SetBombAnimation(std::make_pair(true, it->second.timeStamp));               
                break;
            default:
                break;
            }

            if (!(curDir.x == 0 && curDir.y == 0)) {
                sf::Vector2f newPos = (positionRecalculated ? curPos : (*it).second.currentPos) + ((sf::Vector2f)curDir * ((*it).second.deltaT * playersVelocity));
                if (newPos.x < 50) {
                    newPos.x = 50;
                    positionRecalculated = true;
                }
                if ((*it).second.currentPos.x > 720) {
                    newPos.x = 720;
                    positionRecalculated = true;
                }
                if ((*it).second.currentPos.y < 50) {
                    newPos.y = 50;
                    positionRecalculated = true;
                }
                if ((*it).second.currentPos.y > 520) {
                    newPos.y = 520;
                    positionRecalculated = true;
                }

                if (positionRecalculated)
                    curPos = newPos;
            }
            
        }

        if (positionRecalculated) { //EX:6.1
            sf::Packet p;
            p << static_cast<int>(CommunicationKeys::PosFixed) << curPos.x << curPos.y;
            SendData(client->GetId(), p);
        }

        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::PlayerUpdate);
        clientsPlayingMutex.lock();
        clientsPlaying[client->GetId()]->SetDir(curDir);
        clientsPlaying[client->GetId()]->SetPos(positionRecalculated ? curPos : orderedInputs.rbegin()->second.currentPos);
        p << *clientsPlaying[client->GetId()];
        clientsPlayingMutex.unlock();
       
        SendDataAllButOne(client->GetId(), p);
        
    }});

    //Si un client demana desconectarse el borrem i informem als altres clients
    commandMap.insert({ CommunicationKeys::Disconnect, [this](sf::Packet packet, OnlineClient* client) { //EX:1

        clientsPlayingMutex.lock();
        clientsPlaying.erase(client->GetId());
        clientsPlayingMutex.unlock();

        int idPlayerLeft = client->GetId();
        delete client;
        client = nullptr;

        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::Disconnect) << idPlayerLeft << false;
        SendDataAll(p);
    } });

    commandMap.insert({ CommunicationKeys::Message, [this](sf::Packet packet, OnlineClient* client) { //EX:1
        ChatMessage mssg;
        ChatMessage* mssgToPush;
        packet >> mssg;
        mssgToPush = new ChatMessage(mssg);

        sf::Packet sendPacket;
        sendPacket << static_cast<int>(CommunicationKeys::Message) << mssg;

        messagesHistoryMutex.lock();
        messagesHistory.push_back(mssgToPush);
        messagesHistoryMutex.unlock();
        
        int id = client->GetId();

        SendDataAllButOne(client->GetId(), sendPacket);
    } });
}

Server::~Server()
{   
    clientsPlayingMutex.lock();
    for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++) {
        delete it->second;
        it->second = nullptr;
    }
    clientsPlaying.clear();
    clientsPlayingMutex.unlock();

    bombsInGameMutex.lock();
    for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) {
        delete it->second;
        it->second = nullptr;
    }
    bombsInGame.clear();
    bombsInGameMutex.unlock();

    delete threadReceive;
    threadReceive = nullptr;

    delete udpSocket;
    udpSocket = nullptr;
}

void Server::InitServer()
{
    // Creamos el socket y lo configuramos
    elapsedTime.store(0);
    lastElapsedTime.store(0);
    udpSocket = new sf::UdpSocket();
    udpSocket->setBlocking(true);
    if (udpSocket->bind(5000) != sf::Socket::Done) { //Puerto on escoltar
        std::cout << "Error listening on port 5000" << std::endl;
        return;
    }
    std::cout << "Server is listening on port 5000" << std::endl;

    threadReceive = new std::thread(&Server::HandleServerReceiveData, this); //Thread al bucle per rebre paquetes
    threadReceive->detach();

    HandleGameEvents(); //Thread principal al bucle de controlar la partida
}

void Server::HandleServerReceiveData()
{
    sf::Packet* p = new sf::Packet();
    sf::IpAddress* ipAdr = new sf::IpAddress();
    unsigned short* port = new unsigned short;

    while (!sesionEnd.load()) {
        
        if (udpSocket->receive(*p, *ipAdr, *port) == sf::Socket::Done)
        {
            //std::cout << "Message Received" << std::endl;
            int keyInt;
            *p >> keyInt;
            CommunicationKeys key = static_cast<CommunicationKeys>(keyInt);
            ExecuteCommand(key, *p, key == CommunicationKeys::Login ? GetNewClient(*ipAdr, *port, p) : GetClient(p));
        }
    }
}

void Server::HandleGameEvents()
{
    sf::Clock clock;
    sf::Uint32 subTick = 0, bombSpawnerCounter = 0;
    while (!sesionEnd.load()) {
        elapsedTime = clock.getElapsedTime().asMilliseconds();
        //std::cout << "Elapsed is: " << elapsedTime << std::endl;
        sf::Uint16 deltaTime = elapsedTime - lastElapsedTime;

        //std::cout << elapsedTime << std::endl;

        if (timeRemaining > deltaTime)
            timeRemaining -= deltaTime;
        else
            timeRemaining = 0;

        subTick += deltaTime;
        bombSpawnerCounter += deltaTime;

        if (timeRemaining == 0) {

            CPVector<GamePoints> gameResults;

            clientsPlayingMutex.lock();
            for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++) {
                GamePoints* gP = new GamePoints(it->second->GetName(), it->second->GetPoints());
                gameResults.push_back(gP);
            }
            clientsPlayingMutex.unlock();

            sf::Packet p;
            p << static_cast<int>(CommunicationKeys::GameEnded) << gameResults;
            SendDataAll(p);

            sesionEnd.store(true);
        }
        else {
            if (bombSpawnerCounter >= 10000) { //Cada 10s apareix una bomba //EX:8
                SpawnBomb();
                bombSpawnerCounter = 0;
            }

            if (subTick >= subTickTime) { //Cada subTickTime es revisa l'estat del joc i es demanen els inputs als players pq els enviin
                CheckGameElementsState();
                AskForInputs(); //EX:6.2
                subTick = 0;
            }
        }

        lastElapsedTime = elapsedTime.load();
    }
}

void Server::SendData(int clientID, sf::Packet p)
{
    clientsPlayingMutex.lock();
    udpSocket->send(p, clientsPlaying[clientID]->GetIp(), clientsPlaying[clientID]->GetPort());
    clientsPlayingMutex.unlock();
}

void Server::SendDataAll(sf::Packet p)
{
    clientsPlayingMutex.lock();
    for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++) {
        udpSocket->send(p, it->second->GetIp(), it->second->GetPort());
    }
    clientsPlayingMutex.unlock();
}

void Server::SendDataAllButOne(int clientID, sf::Packet p)
{
    clientsPlayingMutex.lock();
    for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++){
        if(it->second->GetId() != clientID)
            udpSocket->send(p, it->second->GetIp(), it->second->GetPort());
    }
    clientsPlayingMutex.unlock();
}

void Server::SpawnBomb()//EX:8
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(120, 680);
    std::uniform_int_distribution<> distribY(120, 480);
    sf::Vector2f bombSpawn(static_cast<float>(distribX(gen)), static_cast<float>(distribY(gen)));

    bombsInGameMutex.lock();
    int newBombId = bombsID.load();
    bombsID.store(newBombId + 1);
    OnlineBomb* ob = new OnlineBomb(newBombId, -1, bombSpawn, 0);
    bombsInGame.insert({ newBombId, ob});
    bombsInGameMutex.unlock();

    sf::Packet p3;
    p3 << static_cast<int>(CommunicationKeys::BombUpdate) << *ob;
    SendDataAll(p3);
}

sf::Uint32 Server::GetTime()
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

void Server::ExecuteCommand(CommunicationKeys key, sf::Packet data, OnlineClient* clientWhoSentMessage)
{
    auto it = commandMap.find(key);
    if (it != commandMap.end()) {       
        it->second(data, clientWhoSentMessage);
    }
}
/*std::rand() % (680 - 120 + 1) + 120*/  /*std::rand() % (480 - 120 + 1) + 120*/
OnlineClient* Server::GetNewClient(sf::IpAddress ip, unsigned short port, sf::Packet* p)
{
    std::string cName;
    *p >> cName;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribX(120, 680);
    std::uniform_int_distribution<> distribY(120, 480);
    sf::Vector2f playerSpawn(static_cast<float>(distribX(gen)), static_cast<float>(distribY(gen)));

    clientsPlayingMutex.lock();
    playersIdMutex.lock();
    int id = playersID.load();
    OnlineClient* oc = new OnlineClient(ip, port, cName, id, playerSpawn);
    clientsPlaying.insert({ id, oc});
    playersID.store(id + 1);
    playersIdMutex.unlock();
    clientsPlayingMutex.unlock();

    return oc;
}

OnlineClient* Server::GetClient(sf::Packet* p)
{
    int cId;
    *p >> cId;

    OnlineClient* oc;
    clientsPlayingMutex.lock();
    oc = clientsPlaying[cId];
    clientsPlayingMutex.unlock();

    return oc;
}

void Server::AskForInputs()
{
    sf::Packet p;
    p << static_cast<int>(CommunicationKeys::Inputs);
    SendDataAll(p);
}

void Server::CheckGameElementsState()
{
    std::mutex results;

    clientsPlayingMutex.lock();
    std::map<int, std::pair<int, bool>> deadKillerAndWasBomb;
    std::set<int> bombsUpdated;
    std::set<int> bombsToErase;
    std::map<int, std::pair<int, sf::Vector2f>> idPlayerAttackedAndKnockbackInfo;
    std::map<int, int> playersThatWinnedPoints;
    std::multimap<float, int>attacksWithTime;
    std::multimap<float, int>grabsWithTime;
    std::multimap<float, int>throwsWithTime;
    std::set<int> explodeBombs;

    bombsInGameMutex.lock();
    for (auto it = bombsInGame.begin(); it != bombsInGame.end(); it++) {
        if (!it->second->explosionStarted) {
            if (it->second->GetThrewTime() != 0 && elapsedTime - it->second->GetThrewTime() >= 2000) { //Si una bomba s'ha llençat fa mes de 2s comença a explotar //EX:8 //EX:10
                explodeBombs.insert(it->first);
                it->second->explosionStarted = true;
            }
        }
        else if (elapsedTime - it->second->GetThrewTime() >= 2563) { //Si fa mes de 2.563s que sha llençat ya aplica daño als jugadors propers
            if (elapsedTime - it->second->GetThrewTime() >= 3000) { //Si fa mes de 3000 que s'ha llençat la bomba ya ha explotat i borrem la bomba
                bombsToErase.insert(it->first);
            }
            else {
                for (auto it2 = clientsPlaying.begin(); it2 != clientsPlaying.end(); it2++) { //Si encara no ha acabat d'explotar revisem si es pot aplicar daño a algun enemic
                    if (it->second->GetLastOwnerId() == it2->first)
                        continue;

                    float dx = it->second->GetCurrentPos().x - it2->second->GetPos().x;
                    float dy = it->second->GetCurrentPos().y - it2->second->GetPos().y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= 75) {
                        int killerID = it->second->GetLastOwnerId();
                        deadKillerAndWasBomb.insert({ it2->first, { killerID, true } });

                        auto pointsit = playersThatWinnedPoints.find(killerID);
                        if (pointsit != playersThatWinnedPoints.end())
                            pointsit->second += 10;
                        else
                            playersThatWinnedPoints.insert({ killerID, 10 });

                        auto lastIt = clientsPlaying.find(killerID);
                        if (lastIt != clientsPlaying.end()) {
                            lastIt->second->AddPoints(10);
                        }
                    }
                }
            }
        }
        
    }
    bombsInGameMutex.unlock();

    for (auto it = clientsPlaying.begin(); it != clientsPlaying.end(); it++) {
        if (deadKillerAndWasBomb.find(it->first) != deadKillerAndWasBomb.end())
            continue;

        if (it->second->GetAttacking().first) {
            float attackTime = it->second->GetAttacking().second;
            if (elapsedTime - attackTime >= 550)
                it->second->SetAttacking({ false, 0 }); //Si un player estava atacant fa mes de 550ms marquem que ya ha acabat d'atacar
            else
                attacksWithTime.insert({ attackTime, it->first }); //Si encara esta atacant ens el guardem per revisar mestard si ha ferit algu
        }
        else if (it->second->GetBombAnimation().first) {
            float actionTime = it->second->GetBombAnimation().second;
            if (elapsedTime - actionTime >= 550) {
                it->second->SetBombAnimation({ false, 0 }); //Si el player estava agafant o llençant una bomba fa mes de 550s s'acaba la accio
                it->second->actionDone = false;
            }
            else if (!it->second->actionDone){ //Si el player estava agafant o llençant una bomba i encara no haviem processat la accio, ens el guardem per ferho mes endevant
                if (!it->second->hasBomb)
                    grabsWithTime.insert({ actionTime, it->first }); //Playerid //EX:9
                else
                    throwsWithTime.insert({ actionTime, it->first }); //Playerid //EX:10
                it->second->actionDone = true;
            }
        }
    }

    for (auto it = attacksWithTime.begin(); it != attacksWithTime.end(); it++) { //Repassem si la espasa d'algun atacant ha donat a un altre player //EX:7.1
        for (auto it2 = clientsPlaying.begin(); it2 != clientsPlaying.end(); it2++) {
            if (it->second == it2->first || elapsedTime - it2->second->GetLastTimeDMG() <= 550) //Nomes pots fer daño a altres jugadors i ha de fer mes de 550ms que han rebut un cop
                continue;

            sf::Vector2f swordPos(clientsPlaying[it->second]->GetPos());
            sf::Vector2f swordSize;
            switch (clientsPlaying[it->second]->orientation)
            {
            case UP:
                swordPos += sf::Vector2f(16, 0);
                //swordSize = sf::Vector2f(-7, 21);
                swordSize = sf::Vector2f(7, 21);
                break;
            case DOWN:
                swordPos += sf::Vector2f(16, 35);
                //swordSize = sf::Vector2f(7, 21);
                swordSize = sf::Vector2f(7, 21);
                break;
            case LEFT:
                swordPos += sf::Vector2f(-5, 22);
                //swordSize = sf::Vector2f(-21, 7);
                swordSize = sf::Vector2f(21, 7);
                break;
            case RIGHT:
                swordPos += sf::Vector2f(37, 22);
                //swordSize = sf::Vector2f(21, -7);
                swordSize = sf::Vector2f(21, 7);
                break;          
            default:
                break;
            }
            sf::FloatRect boundsSword(sf::FloatRect(swordPos, swordSize));
            sf::FloatRect boundsVictim(it2->second->GetPos(), sf::Vector2f(32, 32)); //Player who could be damaged
            if (boundsSword.intersects(boundsVictim)) { //Si hi ha colisio entra la espasa del atacant i la victima
                int id = it2->second->GetId();
                if (it2->second->GetLives() <= 1) {  //Si ya no li queden vides el guardem amb els altres players que han mort
                    deadKillerAndWasBomb.insert({ id, {it->second, false} });
                    bombsInGameMutex.lock();
                    for (auto it3 = bombsInGame.begin(); it3 != bombsInGame.end(); it3++) { //Si portava una bomba ens la guardem com a que ha sigut llençada
                        if (it3->second->GetOwnerId() == id) {
                            throwsWithTime.insert({ elapsedTime, id });//EX:10 //EX:10.1
                        }
                    }
                    bombsInGameMutex.unlock();

                    auto pointsIt = playersThatWinnedPoints.find(it->second);
                    if (pointsIt != playersThatWinnedPoints.end())
                        pointsIt->second += 4;
                    else
                        playersThatWinnedPoints.insert({ it->second, 4 });

                    auto lastIt = clientsPlaying.find(it->second);
                    if (lastIt != clientsPlaying.end()) {
                        lastIt->second->AddPoints(4);
                    }
                }
                else {
                    it2->second->SetLives(it2->second->GetLives() - 1); //Si encara li queden vides ens el guardem juntament amb la posicio de despres el knockback i el nou numero de vides
                    it2->second->SetLastTimeDMG(it->first);
                    sf::Vector2f knockbackDir = it2->second->GetPos() - clientsPlaying[it->second]->GetPos();
                    float length = std::sqrt(knockbackDir.x * knockbackDir.x + knockbackDir.y * knockbackDir.y);
                    if(length != 0)
                        knockbackDir = sf::Vector2f(knockbackDir.x / length, knockbackDir.y / length);

                    idPlayerAttackedAndKnockbackInfo.insert(std::make_pair(id, std::make_pair(it2->second->GetLives(), knockbackDir))); //EX:7.2

                    auto pointsIt = playersThatWinnedPoints.find(it->second);
                    if (pointsIt != playersThatWinnedPoints.end())
                        pointsIt->second += 2;
                    else
                        playersThatWinnedPoints.insert({ it->second, 2 });

                    auto lastIt = clientsPlaying.find(it->second);
                    if (lastIt != clientsPlaying.end()) {
                        lastIt->second->AddPoints(2);
                    }
                }
            }
        }
    }    

    for (auto it = grabsWithTime.begin(); it != grabsWithTime.end(); it++) { //Revisem totes les accions de recollir per veure quines poden agafar una bomba
        bombsInGameMutex.lock();
        sf::Vector2f bombSize(15 * 1.5f, 15 * 1.5f);
        for (auto it2 = bombsInGame.begin(); it2 != bombsInGame.end(); it2++) {
            if (it2->second->GetOwnerId() != -1 || it2->second->settedFree) //No es poden agafar bombes que porten altres jugadors ni que s'han deixat anar i explotaran
                continue;

            sf::FloatRect boundsBomb(sf::FloatRect(it2->second->GetCurrentPos() + sf::Vector2f(22.5f, 25.f), bombSize));
            sf::FloatRect boundsPlayer(clientsPlaying[it->second]->GetPos() + sf::Vector2f(3, 3), sf::Vector2f(32, 32)); //Player who grabed
            if (boundsPlayer.intersects(boundsBomb)) { //Si es pot recollir una bomba s'actualitza l'estat del player de que porta bomba i el nou propietari d'aquesta
                clientsPlaying[it->second]->hasBomb = true;
                it2->second->SetOwnerId(it->second);
                bombsUpdated.insert(it2->first);
                break;
            }
        }
        bombsInGameMutex.unlock();
    }
    for (auto it = throwsWithTime.begin(); it != throwsWithTime.end(); it++) { //Es revisen totes les accions de llençar bomba //EX:8//EX:10
        bombsInGameMutex.lock();
        for (auto it2 = bombsInGame.begin(); it2 != bombsInGame.end(); it2++) {
            if (it->second == it2->second->GetOwnerId()) { //S'actualitza l'estat de cada player a que no porta bomba (si esta viu) i s'actualitza el propietari de la bomba a cap
                it2->second->SetOwnerId(-1);
                it2->second->SetThrewTime(it->first);
                it2->second->settedFree = true;

                auto it3 = clientsPlaying.find(it->second);
                if (it3 != clientsPlaying.end()) {
                    it3->second->hasBomb = false;
                    it2->second->SetCurrentPos(it3->second->GetPos(), true);
                }
                bombsUpdated.insert(it2->first);
                break;
            }
        }
        bombsInGameMutex.unlock();        
    }
    results.lock();
    clientsPlayingMutex.unlock();
    for (auto it = deadKillerAndWasBomb.begin(); it != deadKillerAndWasBomb.end(); it++) { //Es revisen tots els players que han mort //EX:1 EX:5

        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::Dead);
        SendData((*it).first, p); //Se li comunica al player q esta mort

        clientsPlayingMutex.lock();
        std::string killer = clientsPlaying[(*it).second.first]->GetName();
        std::string victim = clientsPlaying[(*it).first]->GetName();
        delete clientsPlaying[(*it).first];
        clientsPlaying[(*it).first] = nullptr; //Es borra la seva info
        clientsPlaying.erase((*it).first);
        clientsPlayingMutex.unlock();

        sf::Packet p2;
        p2 << static_cast<int>(CommunicationKeys::Disconnect) << (*it).first << true; //S'avisa als altres players de quin jugador a mort
        SendDataAll(p2);

        sf::Packet p3;
        ChatMessage mssg("Log", killer + ((*it).second.second ? " has blown up " : " stabbed " + victim), GetTime(), true);
        p3 << static_cast<int>(CommunicationKeys::Message) << mssg; //S'avisa als altres players de quin jugador a mort
        SendDataAll(p3);
    }
    for (auto it = idPlayerAttackedAndKnockbackInfo.begin(); it != idPlayerAttackedAndKnockbackInfo.end(); it++) { //Es revisen tots els players que han sigut ferits i s'actualizta i s'informa de la seva nova posicio i vida
        clientsPlayingMutex.lock();
        sf::Vector2f newPosAfterKB = clientsPlaying[it->first]->GetPos() + (it->second.second * playersSwordKnockbackForce); //EX:7.2
        clientsPlaying[it->first]->SetPos(newPosAfterKB);
        clientsPlayingMutex.unlock();
        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::Hit) << newPosAfterKB.x << newPosAfterKB.y << clientsPlaying[it->first]->GetLives();
        SendData(it->first, p); 

        sf::Packet p2;
        clientsPlayingMutex.lock();
        p2 << static_cast<int>(CommunicationKeys::PlayerUpdate) << *clientsPlaying[it->first];
        clientsPlayingMutex.unlock();
        SendDataAllButOne(it->first, p2);
    }
    for (auto it = bombsUpdated.begin(); it != bombsUpdated.end(); it++) { //S'envien totes les bombes a les que sels hi ha nodificat algun parametre
        sf::Packet p;
        bombsInGameMutex.lock();
        p << static_cast<int>(CommunicationKeys::BombUpdate) << *bombsInGame[(*it)];
        bombsInGameMutex.unlock();
        SendDataAll(p);
    }
    for (auto it = explodeBombs.begin(); it != explodeBombs.end(); it++) { //S'envien les bombes que comencen a explotar
        sf::Packet p;
        bombsInGameMutex.lock();
        p << static_cast<int>(CommunicationKeys::BombExplode) << (*it);
        bombsInGameMutex.unlock();
        SendDataAll(p);
    }
    for (auto it = bombsToErase.begin(); it != bombsToErase.end(); it++) { //Es borren les bombes que han acabat d'explotar
        bombsInGameMutex.lock();
        bombsInGame.erase((*it));
        bombsInGameMutex.unlock();
    }
    for (auto it = playersThatWinnedPoints.begin(); it != playersThatWinnedPoints.end(); it++) {
        sf::Packet p;
        p << static_cast<int>(CommunicationKeys::Points) << (it->second);
        SendData(it->first, p);
    }
    results.unlock();    
}


