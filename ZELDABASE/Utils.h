#pragma once
#include "ICodable.h"
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include "BombCharacter.h"

enum class CommunicationKeys{ Login, Inputs, MatchData, PosFixed, GameElements, PlayerUpdate, Disconnect, Dead, Hit, BombUpdate, BombExplode, Points, GameEnded, Message};

enum Orientation { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3 };

struct ChatMessage : public ICodable {
	ChatMessage() = default;
	ChatMessage(std::string _clientName, std::string _message, sf::Uint32 _timeStamp, bool _isLog) : clientName(_clientName), message(_message), timeStamp(_timeStamp), isLog(_isLog){}
	std::string message;
	std::string clientName;
	sf::Uint32 timeStamp;	
	bool isLog;

	// Heredado vía ICodable
	void Code(sf::Packet& packet) override {
		packet << clientName << message << timeStamp << isLog;
	}
	void Decode(sf::Packet& packet) override {
		packet >> clientName >> message >> timeStamp >> isLog;
	}
};

struct GamePoints : public ICodable {
	GamePoints() = default;
	GamePoints(std::string _name, int _points) : name(_name), points(_points) {}
	std::string name;
	int points;

	// Heredado vía ICodable
	void Code(sf::Packet& packet) override {
		packet << points << name;
	}
	void Decode(sf::Packet& packet) override {
		packet >> points >> name;
	}
};

struct InputData : public ICodable {
public:
	sf::Uint32 keyCode;
	bool pressed;
	sf::Uint32 timeStamp;
	sf::Uint16 deltaT;
	int idInput;
	sf::Vector2f currentPos;
	InputData() = default;
	InputData(bool _pressed, sf::Uint32 _keyCode, int _idInput, sf::Uint32 _timeStamp, sf::Vector2f _currentPos, sf::Uint16 _deltaT) : pressed(_pressed), keyCode(_keyCode), timeStamp(_timeStamp), idInput(_idInput), currentPos(_currentPos), deltaT(_deltaT) {}

	// Heredado vía ICodable
	void Code(sf::Packet& packet) override {
		packet << pressed << keyCode << timeStamp << currentPos.x << currentPos.y << deltaT;
	}
	void Decode(sf::Packet& packet) override {
		packet >> pressed >> keyCode >> timeStamp >> currentPos.x >> currentPos.y >> deltaT;
	}
};


class OnlineClient : public ICodable {
public:
	OnlineClient() = default;
	OnlineClient(sf::IpAddress _ip, unsigned short _port, std::string _name, int _clientID, sf::Vector2f pos) : ip(_ip), port(_port), name(_name), clientID(_clientID), currentPos(pos) { orientation = Orientation::DOWN; currentDir = sf::Vector2i(0, 0); hasBomb = false; actionDone = false; }
	inline sf::IpAddress GetIp() const { return ip; };
	inline unsigned short GetPort() const { return port; };
	inline std::string GetName() const { return name; };
	inline int GetId() const { return clientID; };
	inline sf::Vector2f GetPos() const { return currentPos; };
	inline void SetPos(sf::Vector2f newPos) { currentPos = newPos; };
	inline sf::Vector2i GetDir() const { return currentDir; };
	inline void SetDir(sf::Vector2i newDir) { currentDir = newDir; };
	inline Orientation GetOri() const { return orientation; };
	inline void SetDir(Orientation newOri) { orientation = newOri; };
	inline std::pair<bool, float> GetAttacking() const { return attackingAndTime; };
	inline void SetAttacking(std::pair<bool, sf::Uint32> newAttack) { attackingAndTime = newAttack; };
	inline std::pair<bool, float> GetBombAnimation() const { return doingBombAnimationAndTime; };
	inline void SetBombAnimation(std::pair<bool, sf::Uint32> newBombAnim) { doingBombAnimationAndTime = newBombAnim; };
	inline int GetLives()const { return lives; }
	inline void SetLives(int newLives) { lives = newLives; }
	inline int GetPoints()const { return points; }
	inline void AddPoints(int pointsToAdd) { points += pointsToAdd; }
	inline sf::Uint32 GetLastTimeDMG()const { return lastTimeDamaged; }
	inline void SetLastTimeDMG(sf::Uint32 newTime) { lastTimeDamaged = newTime; }
	Orientation orientation;
	bool hasBomb;
	bool actionDone;
private:
	sf::Uint32 lastTimeDamaged;
	sf::IpAddress ip;
	unsigned short port;
	std::string name;
	int clientID;
	sf::Vector2f currentPos;
	sf::Vector2i currentDir;
	std::pair<bool, sf::Uint32> attackingAndTime{false, 0};
	std::pair<bool, sf::Uint32> doingBombAnimationAndTime{false, 0};
	int lives = 3, points = 0; //EX:5

	// Heredado vía ICodable
	void Code(sf::Packet& packet) override {
		packet << clientID << name << currentPos.x << currentPos.y << currentDir.x << currentDir.y << static_cast<int>(orientation) << attackingAndTime.first << attackingAndTime.second << lives << hasBomb << doingBombAnimationAndTime.first << doingBombAnimationAndTime.second;
	}
	void Decode(sf::Packet& packet) override {
		int ori;
		packet >> clientID >> name >> currentPos.x >> currentPos.y >> currentDir.x >> currentDir.y >> ori >> attackingAndTime.first >> attackingAndTime.second >> lives >> hasBomb >> doingBombAnimationAndTime.first >> doingBombAnimationAndTime.second;
		orientation = static_cast<Orientation>(ori);
	}
};

class OnlineBomb : public ICodable {
public:
	OnlineBomb() = default;
	OnlineBomb(int _bombId, int _ownerId, sf::Vector2f _currentPos, sf::Uint32 _threwTime) : bombId(_bombId), ownerId(_ownerId), currentPos(_currentPos), threwTime(_threwTime) { CreateSprite(); settedFree = false; lastOwner = -1; }
	void CreateSprite() { bc = new BombCharacter(currentPos); bc->setScale(1.5f, 1.5f); }
	inline int GetBombId() const { return bombId; };
	inline int GetOwnerId() const { return ownerId; };
	inline int GetLastOwnerId() const { return lastOwner; };
	inline void SetOwnerId(int newOwnerId) { if (newOwnerId == -1) lastOwner = ownerId; ownerId = newOwnerId; };
	inline sf::Vector2f GetCurrentPos() const { return currentPos; };
	inline void SetCurrentPos(sf::Vector2f newPos, bool applyOffset) { currentPos = newPos + (applyOffset ? sf::Vector2f(-19,-31) : sf::Vector2f(0,0)); bc->setPosition(newPos + (applyOffset ? sf::Vector2f(-19, -31) : sf::Vector2f(0, 0))); };
	inline sf::Uint32 GetThrewTime() const { return threwTime; };
	inline void SetThrewTime(sf::Uint32 newTime) { threwTime = newTime; };
	/*inline bool GetExplosionStarted() const { return explosionStarted; };
	inline void SetExplosionStarted(bool newStartedState) { explosionStarted = newStartedState; };*/
	BombCharacter* bc;
	bool explosionStarted = false;
	bool settedFree;
private:
	int bombId, ownerId, lastOwner;
	sf::Vector2f currentPos; 
	sf::Uint32 threwTime;
	//bool explosionStarted;
	// Heredado vía ICodable
	void Code(sf::Packet& packet) override {
		packet << bombId << ownerId << currentPos.x << currentPos.y << threwTime << settedFree << lastOwner;
	}
	void Decode(sf::Packet& packet) override {
		packet >> bombId >> ownerId >> currentPos.x >> currentPos.y >> threwTime >> settedFree >> lastOwner;
	}
};