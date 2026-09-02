#pragma once
#include "SimpleMath.h"

struct SceneDataCB
{
    DirectX::SimpleMath::Vector3 viewPos;
    float pad0 = 0.0f;

    DirectX::SimpleMath::Vector2 screenSize;
    DirectX::SimpleMath::Vector2 invScreenSize;

    // x = ssaoEnabled
    // y = ssaoDebugView
    // zw = unused
    DirectX::SimpleMath::Vector4 renderFlags;

    // tile grid LightCullingPass sized its index buffers for this frame
    uint32_t tileCountX = 0;
    uint32_t tileCountY = 0;
};