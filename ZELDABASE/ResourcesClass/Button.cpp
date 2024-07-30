#include "Button.h"

Button::Button(float x, float y)
{
	this->setPosition(x, y);
	this->setOrigin(0, 0);
}

Button::Button(float x, float y, std::string texturePath)
{
	if (!_texture.loadFromFile(texturePath))
	{
		return;
	}
	this->setPosition(x, y);
	this->setTexture(_texture);
	this->setOrigin(0, 0);
}

void Button::SetTexture(std::string t)
{
	if (!_texture.loadFromFile(t))
	{
		return;
	}
	this->setTexture(_texture);
}

bool Button::CheckBounds(unsigned int x, unsigned int y)
{
	return this->getGlobalBounds().contains(x, y);
}
