#pragma once

#include <cstdint>
#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;

struct SkyboxParams
{
    Matrix vp;
    uint32_t flipX;
    uint32_t flipZ;
    uint32_t padding[2];
};

static_assert(sizeof(SkyboxParams) % 16 == 0);