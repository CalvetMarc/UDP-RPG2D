#pragma once

#include <SFML/Graphics.hpp>
#include <functional>

class Button : public sf::Sprite
{
public:

	typedef std::function<void()> OnClick;
	typedef std::function<void()> OnClickRight;
	typedef std::function<void()> OnHoverEnter;
	typedef std::function<void()> OnHoverExit;
	OnClick onClick = []() {};
	OnClickRight onClickRight = []() {};
	OnHoverEnter onHoverEnter = []() {};
	OnHoverExit onHoverExit = []() {};

	Button(float x, float y);
	Button(float x, float y, std::string texturePath);
	~Button() { }

	void SetTexture(std::string t);
	bool CheckBounds(unsigned int x, unsigned int y);

protected:

	sf::Texture _texture;

};