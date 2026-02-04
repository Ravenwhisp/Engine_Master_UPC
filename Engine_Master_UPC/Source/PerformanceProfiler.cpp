#include "Globals.h"
#include "PerformanceProfiler.h"

static TimerMap timers;
static PerfDataMap perfData;

void PerfBegin(const char* name)
{
    timers[name].Start();
}

void PerfEnd(const char* name)
{
    uint64_t ms = timers[name].Stop();

    PerfData& data = perfData[name];
    data.lastMs = (float)ms;
    data.maxMs = std::max(data.maxMs, data.lastMs);
    data.avgMs = (data.avgMs * data.samples + data.lastMs) / (data.samples + 1);
    data.samples++;
}

const PerfDataMap& GetPerfData()
{
    return perfData;
}
