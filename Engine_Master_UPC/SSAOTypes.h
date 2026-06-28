#pragma once

#include <cstdint>
#include <d3d12.h>

#include "SimpleMath.h"
#include "SSAOSettings.h"

struct SSAODataCB
{
    DirectX::SimpleMath::Matrix projection;
    DirectX::SimpleMath::Matrix inverseProjection;

    DirectX::SimpleMath::Vector4 samples[SSAO_KERNEL_SIZE];

    // x = radius
    // y = bias
    // z = strength
    // w = sampleCount
    DirectX::SimpleMath::Vector4 params;

    // x = width
    // y = height
    // z = 1 / width
    // w = 1 / height
    DirectX::SimpleMath::Vector4 screenParams;

    // x = frameIndex
    // yzw = unused
    DirectX::SimpleMath::Vector4 frameParams;
};

struct SSAOFrameData
{
    D3D12_GPU_DESCRIPTOR_HANDLE ssaoSRV{};

    uint32_t width = 0;
    uint32_t height = 0;

    bool enabled = false;
};