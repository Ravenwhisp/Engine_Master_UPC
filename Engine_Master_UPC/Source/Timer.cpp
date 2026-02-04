#include "Globals.h"
#include "Timer.h"

void Timer::Start()
{
	_stop = false;
	_start = steady_clock::now();
}

uint64_t Timer::Read()
{
	if (_stop) {
		return duration_cast<milliseconds>(_end - _start).count();
	}
	else {
		return duration_cast<milliseconds>(steady_clock::now() - _start).count();
	}
}

uint64_t Timer::Stop()
{
	_stop = true;
	_end = steady_clock::now();
	return duration_cast<milliseconds>(_end - _start).count();
}

