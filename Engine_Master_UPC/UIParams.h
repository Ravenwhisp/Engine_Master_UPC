#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;

struct UIParams
{
    Matrix mvp;
    Vector4 fillData;
    float alpha = 1.0f;
	float flipY = 0.0f;
    Vector2 sheetOffset = { 0.0f, 0.0f };
    Vector2 uvScale = { 1.0f, 1.0f };
    Vector2 padding;
};

static_assert(sizeof(UIParams) % 16 == 0);