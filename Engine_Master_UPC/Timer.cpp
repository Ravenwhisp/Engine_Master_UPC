#include "Globals.h"
#include "Timer.h"

void Timer::start()
{
	m_stop = false;
	m_start = steady_clock::now();
}

uint64_t Timer::read()
{
	if (m_stop) 
	{
		return duration_cast<milliseconds>(m_end - m_start).count();
	}
	else 
	{
		return duration_cast<milliseconds>(steady_clock::now() - m_start).count();
	}
}

uint64_t Timer::stop()
{
	m_stop = true;
	m_end = steady_clock::now();
	return duration_cast<milliseconds>(m_end - m_start).count();
}

