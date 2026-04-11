#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;

struct UIParams
{
    Matrix mvp;
    Vector4 fillData;
};

static_assert(sizeof(UIParams) % 16 == 0);