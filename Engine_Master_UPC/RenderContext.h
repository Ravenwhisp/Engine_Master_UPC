#pragma once
#include <d3d12.h>
#include "SimpleMath.h"

class RingBuffer;
class ModuleScene;

struct RenderContext
{
    const DirectX::SimpleMath::Matrix&  view;
    const DirectX::SimpleMath::Matrix&  projection;
    DirectX::SimpleMath::Vector3        cameraPosition;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    RingBuffer*                         ringBuffer = nullptr;
    ModuleScene*                        scene = nullptr;
    bool                                renderDebug = false;
};