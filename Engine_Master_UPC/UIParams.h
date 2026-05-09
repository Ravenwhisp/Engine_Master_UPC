#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;

struct UIParams
{
    Matrix mvp;
    Vector4 fillData;
    float alpha = 1.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

static_assert(sizeof(UIParams) % 16 == 0);