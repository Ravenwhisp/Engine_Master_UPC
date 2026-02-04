#pragma once
#include <string>
#include <unordered_map>
#include "Timer.h"

#define PERF_BEGIN(name) PerfBegin(name)
#define PERF_END(name)   PerfEnd(name)

using TimerMap = std::unordered_map<std::string, Timer>;
using PerfDataMap = std::unordered_map<std::string, struct PerfData>;

struct PerfData
{
	float lastMs = 0.0f;
	float avgMs = 0.0f;
	float maxMs = 0.0f;
	uint32_t samples = 0;
};

void PerfBegin(const char* name);
void PerfEnd(const char* name);

const PerfDataMap& GetPerfData();

