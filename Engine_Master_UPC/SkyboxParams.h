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

struct TextureSize
{
    uint32_t textureSize;
    uint32_t padding[3];
};

static_assert(sizeof(SkyboxParams) % 16 == 0);