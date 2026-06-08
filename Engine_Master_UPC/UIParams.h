#pragma once

#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Vector2;

struct UIParams
{
    Matrix mvp;
    Vector4 fillData;
    Vector2 sheetOffset = { 0.0f, 0.0f };
    Vector2 uvScale = { 1.0f, 1.0f };
	float aspectRatio = 1.0f;
    float alpha = 1.0f;
    Vector2 padding;
};

static_assert(sizeof(UIParams) % 16 == 0);