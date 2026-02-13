#include "Globals.h"
#include "PerformanceProfiler.h"

static TimerMap s_timers;
static PerfDataMap s_performanceData;

void performanceBegin(const char* name)
{
    s_timers[name].start();
}

void performanceEnd(const char* name)
{
    uint64_t ms = s_timers[name].stop();

    PerfData& data = s_performanceData[name];
    data.lastMs = (float)ms;
    data.maxMs = std::max(data.maxMs, data.lastMs);
    data.avgMs = (data.avgMs * data.samples + data.lastMs) / (data.samples + 1);
    data.samples++;
}

const PerfDataMap& getPerfData()
{
    return s_performanceData;
}
