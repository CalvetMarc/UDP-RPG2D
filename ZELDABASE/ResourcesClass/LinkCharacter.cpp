#include "LinkCharacter.h"
#include "ConsoleControl.h"

LinkCharacter::LinkCharacter(sf::Vector2u startPosition, std::string _name, sf::Color teamCol)
{
	this->setPosition(startPosition.x, startPosition.y);
	atlasText = GetSpriteAtlas();
	this->setTexture(*atlasText);
	sword.setTexture(*atlasText);
	sword.setTextureRect(sf::IntRect(52, 285, 7, 21));
	sword.setOrigin(sf::Vector2f(3.5f, 10.5));

	font.loadFromFile("ResourcesClass/Minecraft.ttf");
	name.setFont(font);
	lifes.setFont(font);
	name.setCharacterSize(14);
	lifes.setCharacterSize(14);
	name.setString(_name);
	if (teamCol != sf::Color::Green) {
		this->setColor(teamCol);
		sword.setColor(teamCol);
	}
	name.setFillColor(sf::Color(teamCol.r, teamCol.g, teamCol.b, 255));
	lifes.setFillColor(sf::Color(teamCol.r, teamCol.g, teamCol.b, 255));
}

LinkCharacter::~LinkCharacter()
{	
	delete moveRightAnim;
	moveRightAnim = nullptr;
	delete moveLeftAnim;
	moveLeftAnim = nullptr;
	delete moveUpAnim;
	moveUpAnim = nullptr;
	delete moveDownAnim;
	moveDownAnim = nullptr;

	delete atackRightAnim;
	atackRightAnim = nullptr;
	delete atackLeftAnim;
	atackLeftAnim = nullptr;
	delete atackUpAnim;
	atackUpAnim = nullptr;
	delete atackDownAnim;
	atackDownAnim = nullptr;

	delete chargeRightAnim;
	chargeRightAnim = nullptr;
	delete chargeLeftAnim;
	chargeLeftAnim = nullptr;
	delete chargeUpAnim;
	chargeUpAnim = nullptr;
	delete chargeDownAnim;
	chargeDownAnim = nullptr;

	delete grabRightAnim;
	grabRightAnim = nullptr;
	delete grabLeftAnim;
	grabLeftAnim = nullptr;
	delete grabUpAnim;
	grabUpAnim = nullptr;
	delete grabDownAnim;
	grabDownAnim = nullptr;

	delete throwRightAnim;
	throwRightAnim = nullptr;
	delete throwLeftAnim;
	throwLeftAnim = nullptr;
	delete throwUpAnim;
	throwUpAnim = nullptr;
	delete throwDownAnim;
	throwDownAnim = nullptr;	
}

void LinkCharacter::Draw(sf::RenderWindow* window) //EX:3.1
{
	window->draw(*this);	
	name.setOrigin(name.getGlobalBounds().width * 0.5f, 0);
	name.setPosition(sf::Vector2f(this->getPosition().x + this->getGlobalBounds().width * 0.5f, this->getPosition().y - 20));
	window->draw(name);
	lifes.setString(std::to_string(lifesNum));
	lifes.setOrigin(lifes.getGlobalBounds().width * 0.5f, 0);
	lifes.setPosition(sf::Vector2f(this->getPosition().x + this->getGlobalBounds().width * 0.5f, this->getPosition().y - 40));
	window->draw(lifes);
}

sf::Texture* LinkCharacter::GetSpriteAtlas()
{
	static sf::Texture* spriteAtlas = nullptr;

	if (spriteAtlas == nullptr)
	{
		spriteAtlas = new sf::Texture();
		if (!spriteAtlas->loadFromFile("ResourcesClass/Link.png"))
		{
			ConsoleControl::LockMutex();
			std::cout << "Error al cargar el atlas de link";
			ConsoleControl::UnlockMutex();
		}
		else {
			
		}
	}

	return spriteAtlas;
}
