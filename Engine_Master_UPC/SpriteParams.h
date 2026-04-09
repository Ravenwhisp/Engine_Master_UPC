#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;

struct SpriteParams
{
    Matrix mvp;
};

static_assert(sizeof(SpriteParams) % 16 == 0);