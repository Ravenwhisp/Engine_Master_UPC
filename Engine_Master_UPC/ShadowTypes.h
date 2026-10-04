#pragma once

#include <cstdint>
#include <array>
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
    // Reserved prefix, retained for binary layout compatibility.
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
    Matrix shadowCameraView = Matrix::Identity;
    Vector4 cascadeWorldTexelSize = Vector4::Zero;
    Vector4 cascadeDepthRanges = Vector4::One;
    Vector4 shadowLightDirection = Vector4::Zero;
    uint32_t shadowLightIndex = 0;
    float cascadeBlendFraction = 0.1f;
    float normalBiasTexels = 0.5f;
    float slopeBiasTexels = 1.0f;
};

static_assert(sizeof(ShadowDataCB) == 512, "ShadowDataCB layout must match ShadowData.hlsli.");

struct ShadowFrameData
{
    bool enabled = false;

    D3D12_GPU_VIRTUAL_ADDRESS shadowCBAddress = 0;

    // Independently sized cascade textures; inactive entries alias cascade zero.
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, MAX_SHADOW_CASCADES> cascadeShadowMapSRVs{};
};