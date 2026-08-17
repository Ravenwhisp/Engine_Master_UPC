#pragma once

#include "SimpleMath.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace VolumetricFog
{
    static constexpr uint32_t GRID_WIDTH = 160;
    static constexpr uint32_t GRID_HEIGHT = 90;
    static constexpr uint32_t GRID_DEPTH = 64;
    static constexpr float MIN_DEPTH_RANGE = 0.001f;

    struct GridConstants
    {
        DirectX::SimpleMath::Matrix inverseView = DirectX::SimpleMath::Matrix::Identity;
        DirectX::SimpleMath::Vector2 projectionScale = DirectX::SimpleMath::Vector2::One;
        float nearDistance = 0.1f;
        float maxDistance = 100.0f;
        uint32_t gridWidth = GRID_WIDTH;
        uint32_t gridHeight = GRID_HEIGHT;
        uint32_t gridDepth = GRID_DEPTH;
        uint32_t padding = 0;
    };

    inline float getDepthAtNormalizedZ(float normalizedZ, float nearDistance, float maxDistance)
    {
        const float safeNear = std::max(nearDistance, MIN_DEPTH_RANGE);
        const float safeFar = std::max(maxDistance, safeNear + MIN_DEPTH_RANGE);
        return safeNear * std::pow(safeFar / safeNear, std::clamp(normalizedZ, 0.0f, 1.0f));
    }

    inline float getSliceCenterDepth(uint32_t slice, float nearDistance, float maxDistance)
    {
        const float normalizedZ = (static_cast<float>(slice) + 0.5f) / static_cast<float>(GRID_DEPTH);
        return getDepthAtNormalizedZ(normalizedZ, nearDistance, maxDistance);
    }

    inline float getSliceBoundaryDepth(uint32_t boundary, float nearDistance, float maxDistance)
    {
        const float normalizedZ = static_cast<float>(boundary) / static_cast<float>(GRID_DEPTH);
        return getDepthAtNormalizedZ(normalizedZ, nearDistance, maxDistance);
    }
}