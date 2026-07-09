#include "Globals.h"
#include "UILayoutUtils.h"

#include <algorithm>

namespace
{
    constexpr float k_uiReferenceWidth = 1920.0f;
    constexpr float k_uiReferenceHeight = 1080.0f;
}

namespace UILayoutUtils
{
    Vector2 CalculateScreenSpaceScale(float screenWidth, float screenHeight)
    {
        if (screenWidth <= 0.0f || screenHeight <= 0.0f)
        {
            return { 1.0f, 1.0f };
        }

        const float scaleX = screenWidth / k_uiReferenceWidth;
        const float scaleY = screenHeight / k_uiReferenceHeight;
        const float canvasScale = std::min(scaleX, scaleY);

        return { canvasScale, canvasScale };
    }
}