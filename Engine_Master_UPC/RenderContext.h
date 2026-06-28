#pragma once
#include <d3d12.h>
#include "SimpleMath.h"
#include "RenderViewType.h"
#include "ShadowTypes.h"
#include "SSAOTypes.h"

class RingBuffer;
class Texture;
struct UITextCommand;
struct UIImageCommand;
struct SkyBoxSettings;
struct ParticleEmitterCommand;
class RenderSurface;

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
    const std::vector<ParticleEmitterCommand>* particleCommands = nullptr;

    const SkyBoxSettings* skyBoxSettings = nullptr;

    RenderSurface& renderSurface;
    const ShadowFrameData* shadowData = nullptr;
    Texture* ssaoDepthTexture = nullptr;
    Texture* ssaoNormalTexture = nullptr;
    Texture* ssaoRawTexture = nullptr;
    Texture* ssaoBlurTexture = nullptr;

    const SSAOSettings* ssaoSettings = nullptr;
    const SSAOFrameData* ssaoData = nullptr;

};