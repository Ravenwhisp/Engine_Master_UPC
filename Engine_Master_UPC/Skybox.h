#pragma once
#include <cstdint>
#include "Globals.h"

struct SkyParams
{
    Matrix vp;
    uint32_t flipX;
    uint32_t flipZ;
};

static_assert(sizeof(SkyParams) % 4 == 0);