#include "PBRLighting.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D positionTex : register(t3);
Texture2D emissiveTex : register(t4);

Texture2D ssaoTexture : register(t12);


float SampleSSAO(float4 screenPosition)
{
    if (renderFlags.x < 0.5f)
        return 1.0f;

    float2 ssaoUV = screenPosition.xy * invScreenSize;
    return ssaoTexture.Sample(pointClampSample, ssaoUV).r;
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

    float3 finalColor = ComputePBRSurfaceLighting(worldPos, albedo, metallic, alphaRoughness, ao, emissive, finalWorldNormal, ssao);
    return float4(finalColor, 1.0f);
}