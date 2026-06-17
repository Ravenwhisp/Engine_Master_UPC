#pragma once

#include <cstdint>
#include <d3d12.h>
#include "SimpleMath.h"

using Matrix = DirectX::SimpleMath::Matrix;
using Vector2 = DirectX::SimpleMath::Vector2;

struct ShadowDataCB
{
    Matrix lightViewProjection = Matrix::Identity;

    float shadowBias = 0.0005f;
    float shadowStrength = 1.0f;
    uint32_t shadowsEnabled = 0;
    float padding = 0.0f;

    //PCF
    Vector2 shadowMapTexelSize = Vector2::Zero;
    uint32_t pcfEnabled = 0;
    uint32_t pcfRadius = 1;
};

struct ShadowFrameData
{
    bool enabled = false;

    Matrix lightView = Matrix::Identity;
    Matrix lightProjection = Matrix::Identity;
    Matrix lightViewProjection = Matrix::Identity;

    D3D12_GPU_VIRTUAL_ADDRESS shadowCBAddress = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRV{};
};