#pragma once

#include <list>
#include <mutex>

#include "Button.h"



class Window
{
public:
	typedef std::function<void(Window& w)> Task;
	typedef std::function<void()> OnKeyPressed;
	typedef std::function<void()> OnKeyReleased;

	Window(unsigned int width = 800, unsigned int height = 600, std::string title = "Window");
	void Init();

	std::atomic<bool> initialized = false;

	void AddButton(Button* button);
	void AddDrawable(sf::Drawable* drawable);
	void ClearAllDrawables();
	void ClearButtonsStartingFrom(int position);
	void AddTempDrawable(sf::Drawable* tempDrawable);
	void ClearTempDrawables();

	void RunWindowsLoop();
	unsigned int GetWidth() { return _window.getSize().x; }
	unsigned int GetHeight() { return _window.getSize().y; }

	void CloseWindow();

	//LAMBDA TASKS
	void AddTask(Task task);


	bool IsOpen() const
	{
		return _window.isOpen();
	}


	void SubscribeKeyPressed(sf::Keyboard::Key key, OnKeyPressed onKeyPressed);
	void SubscribeKeyReleased(sf::Keyboard::Key key, OnKeyReleased onKeyReleased);

private:
	//LAMBDA TASK
	std::mutex _taskMutex;
	std::vector<Task> _tasks;

	unsigned int _width, _height;
	std::string _title;

	std::mutex _mutex;
	sf::RenderWindow _window;

	Button* _lastClickedDownButton;
	std::list<Button*> _buttons;
	std::list<sf::Drawable*> _objectsToDraw;
	std::list<sf::Drawable*> _tempObjectsToDraw;
	std::string userTextInput = "";


	std::mutex keyPressedSubscriptionsMutex;
	std::map<sf::Keyboard::Key, OnKeyPressed> keyPressedSubscriptions;
	std::mutex keyReleasedSubscriptionsMutex;
	std::map<sf::Keyboard::Key, OnKeyReleased> keyReleasedSubscriptions;
};