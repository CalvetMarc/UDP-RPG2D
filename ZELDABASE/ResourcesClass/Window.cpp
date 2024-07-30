#include "Window.h"

Window::Window(unsigned int width, unsigned int height, std::string title)
{
	_width = width;
	_height = height;
	_title = title;
}

void Window::Init()
{
	_window.create(sf::VideoMode(_width, _height), _title);
	RunWindowsLoop();
}

void Window::AddButton(Button* button)
{
	_mutex.lock();
	_buttons.push_back(button);
	_objectsToDraw.push_back(button);
	_mutex.unlock();
}

void Window::AddDrawable(sf::Drawable* drawable)
{
	_mutex.lock();
	_objectsToDraw.push_back(drawable);
	_mutex.unlock();
}

void Window::ClearAllDrawables()
{
	_mutex.lock();

	//Exception thrown
	/*
	for (sf::Drawable* object : _objectsToDraw)
	{
		delete object;
	}
	*/

	_objectsToDraw.clear();

	//Exception thrown
	/*
	for (Button* button : _buttons)
	{
		delete button;
	}
	*/

	_buttons.clear();


	_mutex.unlock();
}

void Window::ClearButtonsStartingFrom(int position)
{
	_mutex.lock();

	if (!_buttons.empty())
	{
		std::list<Button*>::iterator it = _buttons.begin();
		std::advance(it, position);

		_buttons.erase(it, _buttons.end());
	}

	_mutex.unlock();
}

void Window::AddTempDrawable(sf::Drawable* tempDrawable)
{
	_mutex.lock();
	_tempObjectsToDraw.push_front(tempDrawable);
	_mutex.unlock();
}

void Window::ClearTempDrawables()
{
	_mutex.lock();
	_tempObjectsToDraw.clear();
	_mutex.unlock();
}

void Window::RunWindowsLoop()
{
	while (_window.isOpen())
	{
		if (!initialized)
			initialized = true;

		_window.clear(sf::Color::Black);

		for (sf::Drawable* drawable : _objectsToDraw)
		{
			if (drawable != nullptr)
			{
				_window.draw(*drawable);
			}
		}


		for (sf::Drawable* tempDrawable : _tempObjectsToDraw)
		{
			if (tempDrawable != nullptr)
			{
				_window.draw(*tempDrawable);
			}
		}

		_window.display();

		sf::Event event;

		while (_window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
			{
				_window.close();
				break;
			}
			case sf::Event::MouseButtonPressed:
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					sf::Event::MouseButtonEvent mouseButton = event.mouseButton;
					sf::Vector2i clickPixelPosition { mouseButton.x, mouseButton.y };
					sf::Vector2f worldPosition = _window.mapPixelToCoords(clickPixelPosition);

					for (auto it = _buttons.rbegin(); it != _buttons.rend(); it++)
					{
						Button* button = *it;
						if (button->CheckBounds(worldPosition.x, worldPosition.y))
						{
							_lastClickedDownButton = button;
							break;
						}
					}
				}
				break;
			}
			case sf::Event::MouseButtonReleased:
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					sf::Event::MouseButtonEvent mouseButton = event.mouseButton;
					sf::Vector2i clickPixelPosition{ mouseButton.x, mouseButton.y };
					sf::Vector2f worldPosition = _window.mapPixelToCoords(clickPixelPosition);

					if (_lastClickedDownButton != nullptr && _lastClickedDownButton->CheckBounds(worldPosition.x, worldPosition.y))
					{
						_lastClickedDownButton->onClick();
						_lastClickedDownButton = nullptr;
					}
					else
					{
						//TODO Cancel click or drag or others
					}
				}
				break;
			}
			case sf::Event::MouseWheelScrolled:
			{
				//TODO
				break;
			}
			case sf::Event::KeyPressed:
			{

				std::map<sf::Keyboard::Key, OnKeyPressed>::iterator it;
				it = keyPressedSubscriptions.find(event.key.code);

				if (it != keyPressedSubscriptions.end())
					it->second();

				break;
			}
			case sf::Event::KeyReleased:
			{
				std::map<sf::Keyboard::Key, OnKeyReleased>::iterator it;
				it = keyReleasedSubscriptions.find(event.key.code);

				if (it != keyReleasedSubscriptions.end())
					it->second;

				break;
			}
			default:
				break;
			}
		}

		//Execute all tasks
		_taskMutex.lock();
		size_t size = _tasks.size();
		for (int i = 0; i < size; i++)
		{
			_tasks[i](*this);
		}
		//Com que són tasques d'un sol ús, cal netejar la llista després del for (ja s'han executat les tasques)
		_tasks.clear();
		_taskMutex.unlock();
	}
}

void Window::CloseWindow()
{
	_window.close();
}

void Window::AddTask(Task task)
{
	_taskMutex.lock();
	_tasks.push_back(task);
	_taskMutex.unlock();
}

void Window::SubscribeKeyPressed(sf::Keyboard::Key key, OnKeyPressed onKeyPressed)
{
	keyPressedSubscriptionsMutex.lock();
	keyPressedSubscriptions[key] = onKeyPressed;
	keyPressedSubscriptionsMutex.unlock();
}

void Window::SubscribeKeyReleased(sf::Keyboard::Key key, OnKeyReleased onKeyReleased)
{
	keyReleasedSubscriptionsMutex.lock();
	keyReleasedSubscriptions[key] = onKeyReleased;
	keyReleasedSubscriptionsMutex.unlock();
}