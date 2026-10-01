#ifndef CASCADE_SHADOW_MAPS_HLSLI
#define CASCADE_SHADOW_MAPS_HLSLI
#include "ShadowData.hlsli"
Texture2D<float> cascadeShadowMap0 : register(t20);
Texture2D<float> cascadeShadowMap1 : register(t21);
Texture2D<float> cascadeShadowMap2 : register(t22);
Texture2D<float> cascadeShadowMap3 : register(t23);
SamplerComparisonState cascadeComparisonSampler : register(s4);
SamplerComparisonState cascadePointSampler : register(s5);

float CompareCascade(uint index, float2 uv, float depth)
{
    if (pcfEnabled == 0)
    {
        if (index == 0)
            return cascadeShadowMap0.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        if (index == 1)
            return cascadeShadowMap1.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        if (index == 2)
            return cascadeShadowMap2.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        return cascadeShadowMap3.SampleCmpLevelZero(cascadePointSampler, uv, depth);
    }

    if (index == 0)
        return cascadeShadowMap0.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
    if (index == 1)
        return cascadeShadowMap1.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
    if (index == 2)
        return cascadeShadowMap2.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
    return cascadeShadowMap3.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
}

float2 GetCascadeTexelSize(uint index)
{
    uint width, height;
    if (index == 0) cascadeShadowMap0.GetDimensions(width, height);
    else if (index == 1) cascadeShadowMap1.GetDimensions(width, height);
    else if (index == 2) cascadeShadowMap2.GetDimensions(width, height);
    else cascadeShadowMap3.GetDimensions(width, height);
    return 1.0f / float2(width, height);
}

bool FilterCascade(uint index, float3 worldPos, out float visibility)
{
    visibility = 1.0f;

    float4 projected = mul(float4(worldPos, 1.0f), cascadeLightViewProjection[index]);
    float3 ndc = projected.xyz / projected.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f)
        return false;

    float2 texelSize = GetCascadeTexelSize(index);
    int radius = pcfEnabled != 0 ? int(pcfRadius) : 0;
    float sum = 0.0f;

    for (int y = -radius; y <= radius; ++y)
        for (int x = -radius; x <= radius; ++x)
            sum += CompareCascade(index, uv + float2(x, y) * texelSize, ndc.z - shadowBias);

    float sampleCount = float((radius * 2 + 1) * (radius * 2 + 1));
    visibility = sum / sampleCount;
    return true;
}

float ComputeShadow(float3 worldPos, float3 normal, out uint selectedCascadeIndex)
{
    selectedCascadeIndex = MAX_SHADOW_CASCADES;

    if (shadowsEnabled == 0)
        return 1.0f;

    uint count = clamp(cascadeCount, 1u, (uint) MAX_SHADOW_CASCADES);

    for (uint index = 0; index < count; ++index)
    {
        float visibility = 1.0f;

        if (FilterCascade(index, worldPos, visibility))
        {
            selectedCascadeIndex = index;
            return lerp(1.0f - shadowStrength, 1.0f, visibility);
        }
    }

    return 1.0f;
}

float3 GetCascadeDebugColor(uint cascadeIndex)
{
    if (cascadeIndex == 0u)
        return float3(1.0f, 0.2f, 0.2f);

    if (cascadeIndex == 1u)
        return float3(0.2f, 1.0f, 0.2f);

    if (cascadeIndex == 2u)
        return float3(0.2f, 0.4f, 1.0f);

    return float3(1.0f, 0.8f, 0.2f);
}


#endif
