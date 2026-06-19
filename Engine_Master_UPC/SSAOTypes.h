#pragma once

#include <cstdint>
#include <d3d12.h>

#include "SimpleMath.h"

static constexpr uint32_t SSAO_KERNEL_SIZE = 32;

struct SSAOSettings
{
    bool enabled = true;

    float radius = 0.5f;
    float bias = 0.025f;
    float strength = 1.0f;

    uint32_t sampleCount = SSAO_KERNEL_SIZE;
};

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