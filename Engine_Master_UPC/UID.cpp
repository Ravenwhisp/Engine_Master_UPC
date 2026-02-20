#include "Globals.h"
#include "UID.h"

#include <chrono>
#include <cstdint>

#include "pcg_basic.h"

static pcg32_random_t g_uidRng;
static bool g_uidRngInit = false;

static void InitUidRng()
{
    if (g_uidRngInit) return;
    g_uidRngInit = true;

    uint64_t seed = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();

    uint64_t seq = (seed ^ 0x9E3779B97F4A7C15ULL) | 1ULL;

    pcg32_srandom_r(&g_uidRng, seed, seq);
}

UID GenerateUID()
{
    InitUidRng();

    uint64_t hi = (uint64_t)pcg32_random_r(&g_uidRng);
    uint64_t lo = (uint64_t)pcg32_random_r(&g_uidRng);
    return (hi << 32) | lo;
}