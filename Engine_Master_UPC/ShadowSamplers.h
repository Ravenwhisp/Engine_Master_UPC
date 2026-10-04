#pragma once
#include <d3d12.h>
#include <array>
#include <cstdint>
inline std::array<D3D12_STATIC_SAMPLER_DESC, 2> makeShadowSamplers(D3D12_SHADER_VISIBILITY visibility)
{
    std::array<D3D12_STATIC_SAMPLER_DESC, 2> samplers{};
    for (uint32_t i = 0; i < 2; ++i)
    {
        auto& s = samplers[i];
        s.Filter = i == 0 ? D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        s.AddressU = s.AddressV = s.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        s.MaxAnisotropy = 1;
        s.MinLOD = s.MaxLOD = 0;
        s.ShaderRegister = 4 + i;
        s.ShaderVisibility = visibility;
    }
    return samplers;
}
