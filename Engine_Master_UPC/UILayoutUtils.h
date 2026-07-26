#pragma once

#include <SimpleMath.h>

using DirectX::SimpleMath::Vector2;

namespace UILayoutUtils
{
    Vector2 CalculateScreenSpaceScale(float screenWidth, float screenHeight);
}