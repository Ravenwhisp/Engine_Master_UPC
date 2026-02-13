#pragma once
#include <chrono>
using namespace std::chrono;

class Timer
{
public:
	void		start();
	uint64_t	read();
	uint64_t	stop();
private:
	time_point<steady_clock> m_start;
	time_point<steady_clock> m_end;
	bool m_stop = false;
};

