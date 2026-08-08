#pragma once

#include <cstdint>
#include <d3d12.h>
#include "SimpleMath.h"

using Matrix = DirectX::SimpleMath::Matrix;
using Vector2 = DirectX::SimpleMath::Vector2;
using Vector4 = DirectX::SimpleMath::Vector4;

static constexpr uint32_t MAX_SHADOW_CASCADES = 4;

enum class ShadowCascadeFitMode : uint32_t
{
    FIT_TO_SCENE = 0,
    FIT_TO_CASCADE = 1
};

struct ShadowDataCB
{
    // Legacy/full fitted frustum.
    // Kept first so the current single-shadow-map pipeline
    // continues working until CSM rendering is enabled.
    Matrix lightViewProjection = Matrix::Identity;

    float shadowBias = 0.0005f;
    float shadowStrength = 1.0f;
    uint32_t shadowsEnabled = 0;
    float padding = 0.0f;

    // PCF
    Vector2 shadowMapTexelSize = Vector2::Zero;
    uint32_t pcfEnabled = 0;
    uint32_t pcfRadius = 1;

    // CSM
    uint32_t cascadeCount = 1;
    uint32_t cascadeFitMode = static_cast<uint32_t>(ShadowCascadeFitMode::FIT_TO_CASCADE);

    Vector2 cascadePadding = Vector2::Zero;

    // View-space far distance of each active cascade.
    Vector4 cascadeFarDistances = Vector4::Zero;

    Matrix cascadeLightViewProjection[MAX_SHADOW_CASCADES] =
    {
        Matrix::Identity,
        Matrix::Identity,
        Matrix::Identity,
        Matrix::Identity
    };
};

static_assert(sizeof(ShadowDataCB) == 384, "ShadowDataCB layout must match the HLSL ShadowDataOutput layout.");

struct ShadowFrameData
{
    bool enabled = false;

    D3D12_GPU_VIRTUAL_ADDRESS shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRV{};
};