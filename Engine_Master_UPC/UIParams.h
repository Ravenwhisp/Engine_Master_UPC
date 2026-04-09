#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;

struct UIParams
{
    Matrix mvp;
};

static_assert(sizeof(UIParams) % 16 == 0);