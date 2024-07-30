#include "ConsoleControl.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

void ConsoleControl::SetColor(ConsoleColor ForeColor, ConsoleColor BackColor)
{
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD color = (BackColor << 4) | ForeColor;
	SetConsoleTextAttribute(hstdout, color);
}

void ConsoleControl::SetPosition(short int x, short int y)
{
	COORD pos = { x, y };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(console, pos);
}

void ConsoleControl::Clear()
{
	ClearCharacter(' ', WHITE, BLACK);
}

void ConsoleControl::ClearCharacter(char character, ConsoleColor ForeColor, ConsoleColor BackColor)
{
	COORD topLeft = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD color = (BackColor << 4) | ForeColor;
	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, character, screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, color,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

void ConsoleControl::Wait(unsigned long milisecons)
{
	Sleep(milisecons);
}

int ConsoleControl::ReadNextKey()
{
	int KB_code = 0;

	if (_kbhit())
	{
		KB_code = _getch();
	}
	return KB_code;
}

int ConsoleControl::WaithForReadNextKey()
{
	int KB_code = 0;

	while (KB_code == 0)
	{
		if (_kbhit())
		{
			KB_code = _getch();
		}
	}

	return KB_code;
}

char ConsoleControl::WaitForReadNextChar()
{
	char c = 0;

	while (c == 0)
	{
		if (_kbhit())
		{
			c = _getch();
		}
	}

	return c;
}

ConsoleControl ConsoleControl::GetInstance() {
	static ConsoleControl instance;

	return instance;
}

void ConsoleControl::LockMutex()
{
	GetInstance().consoleMutex->lock();
}

void ConsoleControl::UnlockMutex()
{
	GetInstance().consoleMutex->unlock();
}
