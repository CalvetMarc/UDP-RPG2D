#pragma once
#include "../Utils.h"
#include "LinkCharacter.h"
#include <vector>
#include "chrono"
#include "set"

class Player
{
public:
	Player(sf::Vector2u startPos/*, sf::Uint32 _connectionTime*/, float vel, std::string _name);
	~Player();
	void Update(sf::Uint16 deltaTime);
	void DrawPlayer(sf::RenderWindow* window);
	void SetLives(int newLives) { playerLink->SetLifes(newLives); }
	void HitReceived(sf::Vector2f pos, int newLives);
	inline void SetPos(sf::Vector2f newPos){ playerLink->setPosition(newPos); }
	inline sf::Vector2f GetPos(){ return playerLink->getPosition(); }
	inline LinkCharacter* GetLink() { return playerLink; }
	inline Animation* GetAnim() { return _currentAnim; }
	std::map<sf::Keyboard::Key, std::function<void(bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime)>> keyEventMap; //Accio de cada input
	bool ProcessInputs(sf::Packet &p);
	inline float GetVelM() { return velMagnitude; }	
	void SetDir(sf::Vector2i newDir) { direction = newDir; }
	sf::Vector2i GetDir() { return direction; }
	void SetOrientation(Orientation newOr) { _orientation = newOr; }
	Orientation GetOrientation() { return _orientation; }
	void RegisterInput(bool pressed, sf::Uint32 keyCode, sf::Uint32 triggerTime, sf::Uint16 deltaT);//Es guarda el input amb la seva info al bufer
	void CancelMovement(int endFrame);
	bool hasBomb = false;
	bool dead = false;
private:	
	bool lastWasUp = true;
	sf::Uint16 lastDeltaTime;
	bool extraReleasedSent = false;
	bool attacking = false;
	bool doingAnimation = false;
	LinkCharacter* playerLink;
	CPVector<InputData> inputBuffer;
	std::mutex bufferMutex;
	float velMagnitude;
	//std::atomic<sf::Uint32>elapsedTime, lastElapsedTime;
	std::atomic<int> inputID;
	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();
	void Attack();
	void Grab();
	void Throw();
	sf::Vector2i direction;
	Orientation _orientation;
	Animation* _currentAnim;
	sf::Vector2f _position;
};