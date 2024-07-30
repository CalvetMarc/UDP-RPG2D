#pragma once
#include "ResourcesClass/LinkCharacter.h"

class BombCharacter : public sf::Sprite
{
public:
	BombCharacter(sf::Vector2f startPosition);
	~BombCharacter();
	void Draw(sf::RenderWindow* window);
	const sf::Vector2i bombSize = sf::Vector2i(15, 15);
	const sf::Vector2i bombSpriteSize = sf::Vector2i(46, 46);
	Animation* currentAnim;
	Animation* staticBomb = new Animation(this, sf::Vector2i(0, 13), bombSpriteSize, 400, 1, 1, 1, false);
	Animation* explosion = new Animation(this, sf::Vector2i(0, 13), bombSpriteSize, 1000, 16, 16, 1, false);
private:
	LinkCharacter* owner;
	sf::Texture* atlasText;
	static sf::Texture* GetSpriteAtlas();
};

