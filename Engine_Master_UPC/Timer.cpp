#include "Globals.h"
#include "Timer.h"

void Timer::start()
{
	m_stop = false;
	m_start = std::chrono::steady_clock::now();
}

uint64_t Timer::read()
{
	if (m_stop) 
	{
		return duration_cast<std::chrono::milliseconds>(m_end - m_start).count();
	}
	else 
	{
		return duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start).count();
	}
}

uint64_t Timer::stop()
{
	m_stop = true;
	m_end = std::chrono::steady_clock::now();
	return duration_cast<std::chrono::milliseconds>(m_end - m_start).count();
}

