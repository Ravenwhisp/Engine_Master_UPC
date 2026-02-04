#pragma once
#include <chrono>
using namespace std::chrono;

class Timer
{
public:
	void Start();
	uint64_t Read();
	uint64_t Stop();
private:
	time_point<steady_clock> _start;
	time_point<steady_clock> _end;
	bool _stop = false;
};

