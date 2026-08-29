#pragma once

#include "SimpleMath.h"
#include <cstdint>

struct FlowMapGPUData
{
    DirectX::SimpleMath::Vector2 direction = DirectX::SimpleMath::Vector2::Zero;
    DirectX::SimpleMath::Vector2 tiling = DirectX::SimpleMath::Vector2::One;
    DirectX::SimpleMath::Vector2 offset = DirectX::SimpleMath::Vector2::Zero;
    float strength = 0.0f;
    uint32_t source = 0;
    uint32_t enabled = 0;
    uint32_t technique = 0;
    float phase = 0.0f;
    float exaggeration = 1.0f;
    uint32_t paddingScalar = 0;
    DirectX::SimpleMath::Vector2 padding = DirectX::SimpleMath::Vector2::Zero;
};
