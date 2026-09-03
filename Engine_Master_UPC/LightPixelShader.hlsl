#include "PBRLighting.hlsli"
#include "LightTileCullingCommon.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D positionTex : register(t3);
Texture2D emissiveTex : register(t4);

Texture2D ssaoTexture : register(t12);

StructuredBuffer<int> pointLightIndices : register(t5);
StructuredBuffer<int> spotLightIndices : register(t6);


float SampleSSAO(float4 screenPosition)
{
    if (renderFlags.x < 0.5f)
        return 1.0f;

    float2 ssaoUV = screenPosition.xy * invScreenSize;
    return ssaoTexture.Sample(pointClampSample, ssaoUV).r;
}

float3 GetLightCountHeatmap(uint count)
{
    if (count == 0)
        return 0.0f;

    const float3 ramp[5] =
    {
        float3(0.0f, 0.0f, 1.0f),
        float3(0.0f, 1.0f, 1.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(1.0f, 1.0f, 0.0f),
        float3(1.0f, 0.0f, 0.0f)
    };

    const float t = saturate(count / 32.0f) * 4.0f;
    const uint i0 = (uint) floor(t);
    const uint i1 = min(i0 + 1, 4);
    return lerp(ramp[i0], ramp[i1], frac(t));
}

uint CountTileLights(uint tileIndex)
{
    const uint tileBase = tileIndex * MAX_LIGHTS_PER_TILE;
    uint count = 0;

    for (uint i = 0; i < MAX_LIGHTS_PER_TILE && pointLightIndices[tileBase + i] >= 0; ++i)
        count++;

    for (uint j = 0; j < MAX_LIGHTS_PER_TILE && spotLightIndices[tileBase + j] >= 0; ++j)
        count++;

    return count;
}

float4 main(float4 position : SV_Position, float2 coord : TEXCOORD0) : SV_TARGET
{
    float3 worldPos = positionTex.Sample(linearWrapSample, coord);
    float3 albedo = baseColorTex.Sample(linearWrapSample, coord);

    float3 metallicRoughnessAOSample = metallicRoughnessTex.Sample(linearWrapSample, coord).rgb;
    float metallic = metallicRoughnessAOSample.b;
    float alphaRoughness = metallicRoughnessAOSample.g;
    float ao = metallicRoughnessAOSample.r;

    float3 emissive = emissiveTex.Sample(linearWrapSample, coord);
    float3 finalWorldNormal = normalTex.Sample(linearWrapSample, coord).rgb;

    float ssao = SampleSSAO(position);

    if (renderFlags.y > 0.5f)
        return float4(ssao.xxx, 1.0f);

    const uint tileIndex = GetTileIndex(uint2(position.xy), tileCount);

    if (renderFlags.z > 0.5f)
        return float4(GetLightCountHeatmap(CountTileLights(tileIndex)), 1.0f);

    float3 finalColor = ComputePBRSurfaceLightingTiled(worldPos, albedo, metallic, alphaRoughness, ao, emissive, finalWorldNormal, ssao, tileIndex, pointLightIndices, spotLightIndices);
    return float4(finalColor, 1.0f);
}
