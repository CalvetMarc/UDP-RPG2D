#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Animation.h"
#include <set>

class LinkCharacter : public sf::Sprite
{
public:

	LinkCharacter(sf::Vector2u startPosition, std::string _name, sf::Color teamCol);
	~LinkCharacter();
	void Draw(sf::RenderWindow* window);
	void SetLifes(int newLifes) { lifesNum = newLifes; lifes.setString(std::to_string(lifesNum)); }
	inline void SetNamePos(sf::Vector2f pos) { name.setPosition(pos); }
	const sf::Vector2i LinkSize = sf::Vector2i(32, 32);
	Sprite sword;
	sf::Vector2f swordOffsetLeft{-5,22};
	sf::Vector2f swordOffsetRight{37,22};
	sf::Vector2f swordOffsetUp{16,0};
	sf::Vector2f swordOffsetDown{16,35};
	
	Animation* moveDownAnim = new Animation(this, sf::Vector2i(8, 72), LinkSize, 400,10, 10, 1, false);
	Animation* moveLeftAnim = new Animation(this, sf::Vector2i(344, 72), LinkSize, 400, 10, 10, 1, false);
	Animation* moveRightAnim = new Animation(this, sf::Vector2i(344, 72), LinkSize, 400, 10, 10, 1, true);
	Animation* moveUpAnim = new Animation(this, sf::Vector2i(674, 72), LinkSize, 400, 10, 10, 1, false);

	Animation* atackDownAnim = new Animation(this, sf::Vector2i(8, 343), LinkSize, 500, 11, 11, 1, false);
	Animation* atackLeftAnim = new Animation(this, sf::Vector2i(379, 343), LinkSize, 500, 10, 10, 1, false);
	Animation* atackRightAnim = new Animation(this, sf::Vector2i(379, 343), LinkSize, 500, 10, 10, 1, true);
	Animation* atackUpAnim = new Animation(this, sf::Vector2i(719, 343), LinkSize, 500, 11, 11, 1, false);

	Animation* chargeDownAnim = new Animation(this, sf::Vector2i(8, 1456), LinkSize, 1000, 10, 10, 1, false);
	Animation* chargeLeftAnim = new Animation(this, sf::Vector2i(349, 1456), LinkSize, 1000, 10, 10, 1, false);
	Animation* chargeRightAnim = new Animation(this, sf::Vector2i(349, 1456), LinkSize, 1000, 10, 10, 1, true);
	Animation* chargeUpAnim = new Animation(this, sf::Vector2i(688, 1456), LinkSize, 1000, 10, 10, 1, false);

	Animation* grabDownAnim = new Animation(this, sf::Vector2i(8, 1411), LinkSize, 500, 5, 5, 1, false);
	Animation* grabLeftAnim = new Animation(this, sf::Vector2i(315, 1411), LinkSize, 500, 5, 5, 1, false);
	Animation* grabRightAnim = new Animation(this, sf::Vector2i(315, 1411), LinkSize, 500, 5, 5, 1, true);
	Animation* grabUpAnim = new Animation(this, sf::Vector2i(624, 1411), LinkSize, 500, 5, 5, 1, false);

	Animation* throwDownAnim = new Animation(this, sf::Vector2i(136, 1411), LinkSize, 500, 5, 5, 1, false);
	Animation* throwLeftAnim = new Animation(this, sf::Vector2i(444, 1411), LinkSize, 500, 5, 5, 1, false);
	Animation* throwRightAnim = new Animation(this, sf::Vector2i(444, 1411), LinkSize, 500, 5, 5, 1, true);
	Animation* throwUpAnim = new Animation(this, sf::Vector2i(752, 1411), LinkSize, 500, 5, 5, 1, false);

	Animation* deadAnim = new Animation(this, sf::Vector2i(8, 876), LinkSize, 500, 5, 5, 1, false);

	int lifesNum = 3;
private:
	sf::Font font;
	sf::Text name, lifes;
	sf::Texture* atlasText;
	static sf::Texture* GetSpriteAtlas();
};

