#include "Player.h"
#include "iostream"

Player::Player(sf::Vector2u startPos, /*sf::Uint32 _connectionTime,*/ float vel, std::string _name) //: GameObject()
{
	//std::cout << "Create player" << std::endl;
	/*elapsedTime = _connectionTime;*/
	velMagnitude = vel;
	playerLink = new LinkCharacter(startPos, _name, sf::Color::Green);
	inputID.store(0);
	_position = sf::Vector2f(startPos.x, startPos.y);
	_orientation = DOWN;
	_currentAnim = playerLink->moveDownAnim;
	_currentAnim->_currentFrame = 6;
	_currentAnim->SetCurrentFrame();
	_currentAnim = nullptr;
	direction = sf::Vector2i(0, 0);	

	keyEventMap.insert({ //EX:6.2
		{ sf::Keyboard::Key::Up, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) { 
			if (attacking || doingAnimation || dead)
				return;
			//if (lastWasUp || !down)
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::Up), triggerTime, deltatTime);
			if (down) {
				//if (lastWasUp)
					MoveUp();
			}				
			else if (_currentAnim == playerLink->moveUpAnim || _currentAnim == playerLink->chargeUpAnim)
				CancelMovement(0);				
			} }, 
		{ sf::Keyboard::Key::Down, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) {
			if (attacking || doingAnimation || dead)
				return;
			//if (lastWasUp || !down)
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::Down), triggerTime, deltatTime);
			if (down) {
				//if (lastWasUp)
					MoveDown();
			}				
			else if (_currentAnim == playerLink->moveDownAnim || _currentAnim == playerLink->chargeDownAnim)
				CancelMovement(6);
			} },
		{ sf::Keyboard::Key::Left, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) {
			if (attacking || doingAnimation || dead)
				return;
			//if (lastWasUp || !down)
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::Left), triggerTime, deltatTime);
			if (down) {
				//if (lastWasUp)
					MoveLeft();
			}					
			else if (_currentAnim == playerLink->moveLeftAnim || _currentAnim == playerLink->chargeLeftAnim)
				CancelMovement(5);
			} },
		{ sf::Keyboard::Key::Right, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) {
			if (attacking || doingAnimation || dead)
				return;
			//if (lastWasUp || !down)
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::Right), triggerTime, deltatTime);
			if (down) {
				//if (lastWasUp)
					MoveRight();
			}							
			else if (_currentAnim == playerLink->moveRightAnim || _currentAnim == playerLink->chargeRightAnim)
				CancelMovement(5);
			} },
		{ sf::Keyboard::Key::Space, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) { //EX:7
			if (down && !attacking && !doingAnimation && !hasBomb && !dead && lastWasUp) {
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::Space), triggerTime, deltatTime);
				Attack();
			}						
		} },
		{ sf::Keyboard::Key::E, [this](bool down, sf::Uint32 triggerTime, sf::Uint16 deltatTime) {
			if (down && !attacking && !doingAnimation && !dead && lastWasUp) {
				RegisterInput(down, static_cast<sf::Uint32>(sf::Keyboard::Key::E), triggerTime, deltatTime);
				if (hasBomb)
					Throw();//EX:10
				else
					Grab(); //EX:9
			}
		} }
	});
	
}


Player::~Player()
{
	_currentAnim = nullptr;

	delete playerLink;
	playerLink = nullptr;
}

void Player::Update(sf::Uint16 deltaTime)
{
	float velMagnitudeScaled = deltaTime * 0.1f;
	sf::Vector2f velVector = ((sf::Vector2f)direction * velMagnitudeScaled);
	if (velVector.x != 0 || velVector.y != 0) {
		sf::Vector2f newPos = playerLink->getPosition() + velVector;
		playerLink->setPosition(newPos);
	}	
}

void Player::DrawPlayer(sf::RenderWindow* window)
{
	if (playerLink != nullptr) {
		if (attacking)
			window->draw(playerLink->sword);
		playerLink->Draw(window);		
	}

}

void Player::HitReceived(sf::Vector2f pos, int newLives)
{
	playerLink->lifesNum = newLives;
	SetPos(pos);
}

bool Player::ProcessInputs(sf::Packet& p)
{	
	bool result = false;
	bufferMutex.lock();
	if (!inputBuffer.empty()) {
		if(inputBuffer.size() > 1)
			extraReleasedSent = false;
		p << inputBuffer;
		inputBuffer.clear();
		inputID.store(0);
		result = true;
	}
	else if(!extraReleasedSent ){ //afegim un release avegades per mitigar un bug
		//std::cout << "Extra Release" << std::endl;
		sf::Keyboard::Key lastKey;
		lastKey = sf::Keyboard::Key::Up;
		switch (_orientation)
		{
		case UP:
			lastKey = sf::Keyboard::Key::Up;
			break;
		case DOWN:
			lastKey = sf::Keyboard::Key::Down;
			break;
		case LEFT:
			lastKey = sf::Keyboard::Key::Left;
			break;	
		case RIGHT:
			lastKey = sf::Keyboard::Key::Right;
			break;
		default:
			return false;
		}
		inputBuffer.push_back(new InputData(false, static_cast<sf::Uint32>(lastKey), 0, 1, playerLink->getPosition(), lastDeltaTime)); //Senvia tot el bufer al server i es buida
		p << inputBuffer;
		inputBuffer.clear();
		extraReleasedSent = true;
		result = true;

	}
	bufferMutex.unlock();

	return result;
}

void Player::MoveUp()
{
	lastWasUp = false;
	_orientation = UP;
	direction = sf::Vector2i(0, -1);

	if (_currentAnim == playerLink->moveUpAnim || _currentAnim == playerLink->chargeUpAnim)
		return;
	else if (_currentAnim != nullptr)
		_currentAnim->Stop();

	if(!hasBomb)
		_currentAnim = playerLink->moveUpAnim;
	else
		_currentAnim = playerLink->chargeUpAnim;

	_currentAnim->Play(true, true);


}

void Player::MoveDown()
{	
	lastWasUp = false;
	_orientation = DOWN;
	direction = sf::Vector2i(0, 1);

	if (_currentAnim == playerLink->moveDownAnim || _currentAnim == playerLink->chargeDownAnim)
		return;
	else if (_currentAnim != nullptr)
		_currentAnim->Stop();

	if(!hasBomb)
		_currentAnim = playerLink->moveDownAnim;
	else
		_currentAnim = playerLink->chargeDownAnim;

	_currentAnim->Play(true, true);
}

void Player::MoveLeft()
{
	lastWasUp = false;
	direction = sf::Vector2i(-1, 0);
	_orientation = LEFT;

	if (_currentAnim == playerLink->moveLeftAnim || _currentAnim == playerLink->chargeLeftAnim)
		return;
	else if (_currentAnim != nullptr)
		_currentAnim->Stop();

	if(!hasBomb)
		_currentAnim = playerLink->moveLeftAnim;
	else
		_currentAnim = playerLink->chargeLeftAnim;

	_currentAnim->Play(false, true);
}

void Player::MoveRight()
{
	lastWasUp = false;
	direction = sf::Vector2i(1, 0);
	_orientation = RIGHT;

	if (_currentAnim == playerLink->moveRightAnim || _currentAnim == playerLink->chargeRightAnim)
		return;
	else if (_currentAnim != nullptr)
		_currentAnim->Stop();

	if(!hasBomb)
		_currentAnim = playerLink->moveRightAnim;
	else
		_currentAnim = playerLink->chargeRightAnim;

	_currentAnim->Play(false, true);
}

void Player::Attack() //EX:7 //EX:7.1
{
	attacking = true;
	direction = sf::Vector2i(0, 0);

	if (_currentAnim != nullptr)
		_currentAnim->Stop();
	
	switch (_orientation)
	{
	case UP:
		_currentAnim = playerLink->atackUpAnim;
		playerLink->sword.setRotation(0);
		playerLink->sword.setPosition(playerLink->getPosition() + playerLink->swordOffsetUp);
		break;
	case DOWN:
		_currentAnim = playerLink->atackDownAnim;
		playerLink->sword.setRotation(180);
		playerLink->sword.setPosition(playerLink->getPosition() + playerLink->swordOffsetDown);
		break;
	case LEFT:
		_currentAnim = playerLink->atackLeftAnim;
		playerLink->sword.setRotation(-90);
		playerLink->sword.setPosition(playerLink->getPosition() + playerLink->swordOffsetLeft);
		break;
	case RIGHT:
		_currentAnim = playerLink->atackRightAnim;
		playerLink->sword.setRotation(90);
		playerLink->sword.setPosition(playerLink->getPosition() + playerLink->swordOffsetRight);
		break;
	default:
		break;
	}
	_currentAnim->PlayOnce([this]() {
		attacking = false; 
		int frame = 0;		
		switch (_orientation)
		{
		case UP:
			_currentAnim = playerLink->moveUpAnim;
			frame = 0;
			break;
		case DOWN:
			_currentAnim = playerLink->moveDownAnim;
			frame = 6;
			break;
		case LEFT:
			_currentAnim = playerLink->moveLeftAnim;
			frame = 5;
			break;
		case RIGHT:
			_currentAnim = playerLink->moveRightAnim;
			frame = 5;
			break;
		default:
			break;
		}
		_currentAnim->_currentFrame = frame;
		_currentAnim->SetCurrentFrame();
		_currentAnim = nullptr;
	});
}

void Player::Grab()//EX:9
{
	doingAnimation = true;
	direction = sf::Vector2i(0, 0);
	if (_currentAnim != nullptr)
		_currentAnim->Stop();

	switch (_orientation)
	{
	case UP:
		_currentAnim = playerLink->grabUpAnim;		
		break;
	case DOWN:
		_currentAnim = playerLink->grabDownAnim;		
		break;
	case LEFT:
		_currentAnim = playerLink->grabLeftAnim;		
		break;
	case RIGHT:
		_currentAnim = playerLink->grabRightAnim;		
		break;
	default:
		break;
	}
	_currentAnim->PlayOnce([this]() {
		doingAnimation = false;
		
		int frame = 0;
		switch (_orientation)
		{
		case UP:
			if (!hasBomb) {
				_currentAnim = playerLink->moveUpAnim;
				frame = 0;
			}
			else {
				_currentAnim = playerLink->grabUpAnim;
				frame = 4;
			}
			break;
		case DOWN:
			if (!hasBomb) {
				_currentAnim = playerLink->moveDownAnim;
				frame = 6;
			}
			else {
				_currentAnim = playerLink->grabDownAnim;
				frame = 4;
			}			
			break;
		case LEFT:
			if (!hasBomb) {
				_currentAnim = playerLink->moveLeftAnim;
				frame = 5;
			}
			else {
				_currentAnim = playerLink->grabLeftAnim;
				frame = 4;
			}
			break;
		case RIGHT:
			if (!hasBomb) {
				_currentAnim = playerLink->moveRightAnim;
				frame = 5;
			}
			else {
				_currentAnim = playerLink->grabRightAnim;
				frame = 4;
			}
			break;
		default:
			break;
		}
		_currentAnim->_currentFrame = frame;
		_currentAnim->SetCurrentFrame();
		_currentAnim = nullptr;
	});
}

void Player::Throw()//EX:10
{
	hasBomb = false;
	doingAnimation = true;
	direction = sf::Vector2i(0, 0);
	if (_currentAnim != nullptr)
		_currentAnim->Stop();

	switch (_orientation)
	{
	case UP:
		_currentAnim = playerLink->throwUpAnim;
		break;
	case DOWN:
		_currentAnim = playerLink->throwDownAnim;
		break;
	case LEFT:
		_currentAnim = playerLink->throwLeftAnim;
		break;
	case RIGHT:
		_currentAnim = playerLink->throwRightAnim;
		break;
	default:
		break;
	}
	_currentAnim->PlayOnce([this]() {
		doingAnimation = false;
		int frame = 0;
		switch (_orientation)
		{
		case UP:
			_currentAnim = playerLink->moveUpAnim;
			frame = 0;
			break;
		case DOWN:
			_currentAnim = playerLink->moveDownAnim;
			frame = 6;
			break;
		case LEFT:
			_currentAnim = playerLink->moveLeftAnim;
			frame = 5;
			break;
		case RIGHT:
			_currentAnim = playerLink->moveRightAnim;
			frame = 5;
			break;
		default:
			break;
		}
		_currentAnim->_currentFrame = frame;
		_currentAnim->SetCurrentFrame();
		_currentAnim = nullptr;
		});
}

void Player::RegisterInput(bool pressed, sf::Uint32 keyCode, sf::Uint32 triggerTime, sf::Uint16 deltaT) //EX:6.2
{
	int auxId = inputID.load();
	InputData* inptData = new InputData(pressed, keyCode, auxId, triggerTime, playerLink->getPosition(), deltaT);
	inputID.store(auxId + 1);

	bufferMutex.lock();
	inputBuffer.push_back(inptData);
	bufferMutex.unlock();
}

void Player::CancelMovement(int endFrame)
{
	lastWasUp = true;
	direction = sf::Vector2i(0, 0);
	_currentAnim->Stop();
	_currentAnim->_currentFrame = endFrame;
	_currentAnim->SetCurrentFrame();
	_currentAnim = nullptr;
}

