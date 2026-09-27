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
        if (index == 0) return cascadeShadowMap0.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        if (index == 1) return cascadeShadowMap1.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        if (index == 2) return cascadeShadowMap2.SampleCmpLevelZero(cascadePointSampler, uv, depth);
        return cascadeShadowMap3.SampleCmpLevelZero(cascadePointSampler, uv, depth);
    }
    if (index == 0) return cascadeShadowMap0.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
    if (index == 1) return cascadeShadowMap1.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
    if (index == 2) return cascadeShadowMap2.SampleCmpLevelZero(cascadeComparisonSampler, uv, depth);
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

bool FilterCascade(uint index, float3 worldPos, float3 normal, out float visibility)
{
    visibility = 1.0f;
    float cosine = saturate(dot(normal, -shadowLightDirection.xyz));
    bool surface = dot(normal, normal) > 0.5f;
    float sine = surface ? sqrt(saturate(1.0f - cosine * cosine)) : 0.0f;
    float worldTexel = cascadeWorldTexelSize[index];
    float3 biasedPosition = worldPos + normal * (normalBiasTexels * worldTexel * sine);
    float4 projected = mul(float4(biasedPosition, 1), cascadeLightViewProjection[index]);
    float3 ndc = projected.xyz / projected.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
    float2 texel = GetCascadeTexelSize(index);
    int radius = pcfEnabled != 0 ? int(pcfRadius) : 0;
    // Require the entire filter footprint. Invalid coverage falls back to a coarser cascade.
    float2 margin = (float(radius) + 0.5f) * texel;
    if (any(uv < margin) || any(uv > 1.0f - margin) || ndc.z < 0 || ndc.z > 1)
        return false;
    float slope = surface ? min(sine / max(cosine, 0.2f), 2.0f) : 0.0f;
    // Legacy bias is interpreted in cascade-zero texels, avoiding depth-range-dependent detachment.
    float baseBiasTexels = shadowBias / max(shadowMapTexelSize.x, 0.000001f);
    float bias = (baseBiasTexels + slopeBiasTexels * slope) * worldTexel / max(cascadeDepthRanges[index], 0.0001f);
    float sum = 0;
    for (int y = -radius; y <= radius; ++y)
        for (int x = -radius; x <= radius; ++x)
            sum += CompareCascade(index, uv + float2(x, y) * texel, ndc.z - bias);
    visibility = sum / float((2 * radius + 1) * (2 * radius + 1));
    return true;
}

float ComputeShadow(float3 worldPos, float3 normal, out uint selectedCascadeIndex)
{
    selectedCascadeIndex = MAX_SHADOW_CASCADES;
    if (shadowsEnabled == 0) return 1.0f;
    uint count = clamp(cascadeCount, 1u, (uint)MAX_SHADOW_CASCADES);
    float depth = -mul(float4(worldPos, 1), shadowCameraView).z;
    if (depth < cascadePadding.y || depth > cascadeFarDistances[count - 1]) return 1.0f;
    uint index = 0;
    while (index + 1 < count && depth > cascadeFarDistances[index]) ++index;
    float visibility = 1;
    bool valid = false;
    for (uint candidate = index; candidate < count; ++candidate)
    {
        if (FilterCascade(candidate, worldPos, normal, visibility))
        {
            index = candidate;
            selectedCascadeIndex = candidate;
            valid = true;
            break;
        }
    }
    if (!valid) return 1.0f;
    float nearDepth = index == 0 ? cascadePadding.y : cascadeFarDistances[index - 1];
    float blendWidth = max((cascadeFarDistances[index] - nearDepth) * cascadeBlendFraction, 0.0001f);
    float blend = cascadeBlendFraction > 0 ? saturate((depth - cascadeFarDistances[index] + blendWidth) / blendWidth) : 0;
    if (blend > 0)
    {
        float nextVisibility = 1;
        if (index + 1 == count || FilterCascade(index + 1, worldPos, normal, nextVisibility))
            visibility = lerp(visibility, nextVisibility, blend);
    }
    return lerp(1.0f - shadowStrength, 1.0f, visibility);
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
