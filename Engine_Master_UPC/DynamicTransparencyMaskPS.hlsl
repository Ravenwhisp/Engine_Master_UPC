#include "DynamicTransparencyCommon.hlsli"

#define DYNAMIC_TRANSPARENCY_PROBES 3
cbuffer DynamicTransparencyMaskCB : register(b0)
{
    float4 centerRadius[MAX_DYNAMIC_TRANSPARENCY_TARGETS];
    float4 depthSoftness[MAX_DYNAMIC_TRANSPARENCY_TARGETS];
    float4 probes[MAX_DYNAMIC_TRANSPARENCY_TARGETS * DYNAMIC_TRANSPARENCY_PROBES];
    float4 settings;
};

// Nearest occluder surface, rendered before any transparency discard.
Texture2D<float> occluderDepthTexture : register(t0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float TargetOcclusion(uint target)
{
    uint width, height;
    occluderDepthTexture.GetDimensions(width, height);
    float coverage = 0.0f;
    for (uint p = 0; p < DYNAMIC_TRANSPARENCY_PROBES; ++p)
    {
        float4 probe = probes[target * DYNAMIC_TRANSPARENCY_PROBES + p];
        if (any(probe.xy < 0.0f) || probe.x >= float(width) || probe.y >= float(height))
            continue;
        float wallDepth = occluderDepthTexture.Load(int3(int2(probe.xy), 0));
        // World-space margin projected separately for this camera and probe.
        coverage += smoothstep(probe.w, 2.0f * probe.w, probe.z - wallDepth);
    }
    // A single occluded probe must not open the wall.
    return smoothstep(0.34f, 1.0f, coverage / DYNAMIC_TRANSPARENCY_PROBES);
}

float4 main(PSInput input) : SV_Target0
{
    uint targetCount = min((uint) settings.x, (uint) MAX_DYNAMIC_TRANSPARENCY_TARGETS);
    float4 result = 0.0f;
    for (uint i = 0; i < targetCount; ++i)
    {
        float2 offset = (input.position.xy - centerRadius[i].xy) / max(centerRadius[i].zw, 1.0f);
        float radius = length(offset);
        if (radius >= 1.0f)
            continue;
        float activation = TargetOcclusion(i);
        if (activation <= 0.0f)
            continue;

        // Fully clear core, with the existing blended wall pass at its border.
        // The stable proxy replaces the animated silhouette, also at the core.
        float edgeStart = min(depthSoftness[i].z, 1.0f - max(depthSoftness[i].y, 0.001f));
        float influence = activation * (1.0f - smoothstep(edgeStart, 1.0f, radius));
        float2 data = float2(influence, depthSoftness[i].x);
        if (i == 0)
            result.xy = data;
        else
            result.zw = data;
    }
    return result;
}
