#pragma once
#include <d3d12.h>
#include "SimpleMath.h"
#include "RenderViewType.h"

class RingBuffer;
struct UITextCommand;
struct UIImageCommand;
struct SkyBoxSettings;

struct RenderContext
{
    const DirectX::SimpleMath::Matrix&  view;
    const DirectX::SimpleMath::Matrix&  projection;
    DirectX::SimpleMath::Vector3        cameraPosition;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    RingBuffer*                         ringBuffer = nullptr;
    bool                                renderDebug = false;
    RenderViewType viewType = RenderViewType::Game;

    const std::vector<UITextCommand>* uiTextCommands = nullptr;
    const std::vector<UIImageCommand>* uiImageCommands = nullptr;

    const SkyBoxSettings* skyBoxSettings = nullptr;

};