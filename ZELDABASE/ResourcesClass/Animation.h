#pragma once
#include <SFML/Graphics.hpp>
#include <thread>
#include <mutex>
#include <functional>

class Animation
{
public:
	typedef std::function<void()> OnAnimationEnd;
public:
	Animation(sf::Sprite* target, sf::Vector2i startOffset, sf::Vector2i size, unsigned long time, int framesCount, int columns, int rows, bool isReversedSprite);
	Animation();
	~Animation();

	void SetCurrentFrame();
	int _currentFrame = 0;

	void Play(bool fromStart = false, bool loop = false);
	void Stop();
	void PlayOnce(OnAnimationEnd onAnimationEnd);

	void ResetFrame();
	bool NextFrame();

private:
	sf::Sprite* _target = nullptr;
	sf::Vector2i _startOffset;
	sf::Vector2i _size;
	unsigned long _time;
	int _framesCount;
	int _columns;
	int _rows;
	bool _isReversedSprite;

	sf::IntRect _rect;

	void StartAnimationTimer();

	std::mutex _mutex;
	bool _playing = false;

	unsigned long _updateFrameTime = 100;

	bool _isPlayOnce = false;
	OnAnimationEnd _onAnimationEnd;
};

