#pragma once
#include "SimpleMath.h"

static constexpr UINT MAX_DYNAMIC_TRANSPARENCY_TARGETS = 2;
static constexpr UINT DYNAMIC_TRANSPARENCY_PROBES = 3;

// Keep in sync with DynamicTransparencyMaskPS.hlsl. Each target owns its
// visibility probes and depth; overlapping targets must not share a depth.
struct DynamicTransparencyMaskCB
{
    DirectX::SimpleMath::Vector4 centerRadius[MAX_DYNAMIC_TRANSPARENCY_TARGETS]{};
    DirectX::SimpleMath::Vector4 depthSoftness[MAX_DYNAMIC_TRANSPARENCY_TARGETS]{};
    DirectX::SimpleMath::Vector4 probes[MAX_DYNAMIC_TRANSPARENCY_TARGETS * DYNAMIC_TRANSPARENCY_PROBES]{};
    DirectX::SimpleMath::Vector4 settings{};
};
static_assert(sizeof(DynamicTransparencyMaskCB) == 176);
