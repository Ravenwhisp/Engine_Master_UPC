#include "PBRLighting.hlsli"
#include "DynamicTransparencyFalloffCommon.hlsli"

cbuffer FalloffSettingsCB : register(b5)
{
    float4 falloffSettings;
};

Texture2D diffuseTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D emissiveTex : register(t3);

Texture2D<float2> dynamicTransparencyMask : register(t12);
Texture2D dissolveNoise : register(t13);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

float4 main(PSInput input) : SV_Target
{
    int2 pixelCoord = int2(input.position.xy);
    float2 transparencyData = dynamicTransparencyMask.Load(int3(pixelCoord, 0));

    float influence = transparencyData.r;
    float targetDepth = transparencyData.g;

    const float coreThreshold = 0.999f;

    float depthBias = falloffSettings.x;
    bool hasDissolveComponent = falloffSettings.y > 0.5f;
    float dissolveAmount = falloffSettings.z;

    if (influence <= 0.0f)
        discard;

    if (influence >= coreThreshold)
        discard;

    if (input.position.z >= targetDepth - depthBias)
        discard;

    float metallic = material.metallicFactor;
    float alphaRoughness = material.roughnessFactor;
    float ao = 1.0f;
    float3 emissive = 0.0f;
    float3 finalWorldNormal = normalize(input.normal);

    float3 albedo = material.diffuseColour;

    if (material.hasDiffuseTex != 0)
    {
        float4 diffuseSample = diffuseTex.Sample(linearWrapSample, input.texCoord);

        if (diffuseSample.a < 0.5f)
            discard;

        albedo *= diffuseSample.rgb;
    }

    if (hasDissolveComponent && dissolveAmount > 0.0f)
    {
        float noise = dissolveNoise.Sample(linearWrapSample, input.texCoord).r;

        if (noise <= dissolveAmount)
            discard;
    }

    if (material.hasMetallicRoughnessTex != 0)
    {
        float4 metallicRoughnessSample = metallicRoughnessTex.Sample(linearWrapSample, input.texCoord);

        metallic = saturate(metallicRoughnessSample.b * material.metallicFactor);
        ao = metallicRoughnessSample.r;
        alphaRoughness = 1.0f - clamp(metallicRoughnessSample.g, 0.04f, 1.0f);
    }

    if (material.hasNormalTex != 0)
    {
        float3 tangentNormal = normalTex.Sample(linearWrapSample, input.texCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0f - 1.0f);

        float3 tangentVector = normalize(input.tangent);
        float3 bitangentVector = cross(finalWorldNormal, tangentVector);
        float3x3 TBN = float3x3(tangentVector, bitangentVector, finalWorldNormal);

        finalWorldNormal = mul(tangentNormal, TBN);
    }

    if (material.hasEmissiveTex != 0)
    {
        float3 emissiveSample = emissiveTex.Sample(linearWrapSample, input.texCoord).rgb;
        emissive = emissiveSample * material.emmisiveColour;
    }

    // Do NOT use screen-space SSAO here.
    // MainDepth belongs to the real scene behind the transparent occluder.
    float3 finalColor = ComputePBRSurfaceLighting(input.worldPos, albedo, metallic, alphaRoughness, ao, emissive, finalWorldNormal, 1.0f);

    float wallOpacity = saturate(1.0f - influence);

    return float4(finalColor, wallOpacity);
}