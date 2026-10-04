#pragma once
#include <d3d12.h>
#include <array>
#include <cstdint>


inline std::array<D3D12_STATIC_SAMPLER_DESC, 2> makeShadowSamplers(D3D12_SHADER_VISIBILITY visibility)
{
    std::array<D3D12_STATIC_SAMPLER_DESC, 2> samplers{};

    samplers[0].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 4;
    samplers[0].ShaderVisibility = visibility;

    samplers[1] = samplers[0];
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    samplers[1].ShaderRegister = 5;

    return samplers;
}