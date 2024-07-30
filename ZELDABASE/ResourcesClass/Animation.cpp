#include "Animation.h"
#include "Timer.h"

Animation::Animation(sf::Sprite* target, sf::Vector2i startOffset, sf::Vector2i size, unsigned long time, int framesCount, int columns, int rows, bool isReversedSprite)
{
	this->_target = target;
	this->_startOffset = startOffset;
	this->_size = size;
	this->_time = time;
	this->_framesCount = framesCount;
	this->_columns = columns;
	this->_rows = rows;
	this->_isReversedSprite = isReversedSprite;

	this->_updateFrameTime = time / framesCount;

	if (_isReversedSprite)
	{
		_startOffset.x += size.x;
	}

	_rect = sf::IntRect(_startOffset, _size);
}

Animation::Animation()
{
}

Animation::~Animation()
{
	Stop();
}

void Animation::Play(bool fromStart, bool loop)
{
	_mutex.lock();

	if (fromStart)
	{
		ResetFrame();
	}	

	bool isPlaying = _playing;
	if (!isPlaying)
	{
		_playing = true;
	}
	_mutex.unlock();
	
	if (!isPlaying)
	{
		StartAnimationTimer();
	}
}

void Animation::Stop()
{
	_mutex.lock();
	_playing = false;
	_mutex.unlock();
}

void Animation::PlayOnce(OnAnimationEnd onAnimationEnd)
{
	_onAnimationEnd = onAnimationEnd;
	_isPlayOnce = true;
	Play(true);
}

void Animation::ResetFrame()
{
	_currentFrame = 0;
	SetCurrentFrame();
}

bool Animation::NextFrame()
{
	if (_currentFrame >= _framesCount - 1)
	{
		_currentFrame = 0;

		if (_isPlayOnce)
		{
			return true;
		}
	}
	else
	{
		_currentFrame++;
	}

	SetCurrentFrame();
	return false;
}

void Animation::StartAnimationTimer()
{
	Timer::StartLoopTimer(_updateFrameTime, [this]() {

		_mutex.lock();
		bool isPlaying = _playing;
		_mutex.unlock();

		if (isPlaying)
		{
			bool needFinish = this->NextFrame();
			_mutex.lock();
			isPlaying = !needFinish;
			_playing = isPlaying;
			_mutex.unlock();
			if (needFinish)
			{
				_onAnimationEnd();
			}
		}

		return isPlaying;
	});
}

void Animation::SetCurrentFrame()
{
	int col;
	int row;

	row = _currentFrame / _columns;
	col = _currentFrame % _columns;

	sf::Vector2i size = _size;

	if (_isReversedSprite)
	{
		size.x = -size.x;
	}

	_rect = sf::IntRect(_startOffset + sf::Vector2i((_size.x * col), (_size.y * row)), size);
	_target->setTextureRect(_rect);
}


