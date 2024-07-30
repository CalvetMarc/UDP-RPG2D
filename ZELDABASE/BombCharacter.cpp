#include "BombCharacter.h"
#include "iostream"

BombCharacter::BombCharacter(sf::Vector2f startPosition)
{
	this->setPosition(startPosition.x, startPosition.y);
	atlasText = GetSpriteAtlas();
	this->setTexture(*atlasText);
	currentAnim = staticBomb;
	currentAnim->_currentFrame = 0;
	currentAnim->SetCurrentFrame();
	currentAnim = nullptr;
}

BombCharacter::~BombCharacter()
{
	delete currentAnim;
	currentAnim = nullptr;
	delete staticBomb;
	staticBomb = nullptr;
	delete explosion;
	explosion = nullptr;
		
	owner = nullptr;
	delete atlasText;
	atlasText = nullptr;
}

void BombCharacter::Draw(sf::RenderWindow* window)
{
	window->draw(*this);
}

sf::Texture* BombCharacter::GetSpriteAtlas()
{
	static sf::Texture* spriteAtlas = nullptr;

	if (spriteAtlas == nullptr)
	{
		spriteAtlas = new sf::Texture();
		if (!spriteAtlas->loadFromFile("ResourcesClass/Bombs.png"))
		{
			std::cout << "Error al cargar la bomba" << std::endl;
		}
		else {

		}
	}

	return spriteAtlas;
}
