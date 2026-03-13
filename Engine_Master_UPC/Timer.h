#pragma once

#include <chrono>
#include <cstdint>

class Timer
{
public:
    void start();
    uint64_t read();
    uint64_t stop();

private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_end;
    bool m_stop = false;
};