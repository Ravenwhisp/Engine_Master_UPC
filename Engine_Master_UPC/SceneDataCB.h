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
};