#include "Timer.h"
#include <thread>
#include <windows.h>

void Timer::StartTimer(unsigned long timeRequiredMilisecons, OnTimeElapsed onTimeElapse)
{
	std::thread* thread = new std::thread([timeRequiredMilisecons,onTimeElapse]() {
			Sleep(timeRequiredMilisecons);
			onTimeElapse();
		});

	thread->detach();
}

void Timer::StartLoopTimer(unsigned long timeRequiredMilisecons, OnTimeElapsedWithLoop onTimeElapse)
{
	std::thread* thread = new std::thread([timeRequiredMilisecons, onTimeElapse]() {

		bool continueLoop = true;
		while (continueLoop)
		{
			Sleep(timeRequiredMilisecons);
			continueLoop = onTimeElapse();
		}
		
		});

	thread->detach();
}
