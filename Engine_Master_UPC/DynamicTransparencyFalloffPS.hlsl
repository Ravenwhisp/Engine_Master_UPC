#include "PBRLighting.hlsli"
#include "DynamicTransparencyFalloffCommon.hlsli"
#include "DynamicTransparencyCommon.hlsli"

cbuffer FalloffSettingsCB : register(b5)
{
    float4 falloffSettings;
    float4 fogDepthParams;
    float4 fogGridParams;
};

Texture2D diffuseTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D emissiveTex : register(t3);

Texture2D<float4> dynamicTransparencyMask : register(t12);
Texture2D dissolveNoise : register(t13);
Texture3D<float4> integratedFogVolume : register(t14);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

float LinearizeFogViewDepth(float deviceDepth)
{
    float projectionA = fogDepthParams.z;
    float projectionB = fogDepthParams.w;

    float denominator = deviceDepth + projectionA;

    if (abs(denominator) < 0.000001f)
        denominator = denominator < 0.0f ? -0.000001f : 0.000001f;

    float viewZ = -projectionB / denominator;

    return max(-viewZ, 0.0f);
}

float GetNormalizedFogDepth(float viewDepth)
{
    float nearDistance = fogDepthParams.x;
    float maxDistance = fogDepthParams.y;

    float safeNear = max(nearDistance, 0.001f);
    float safeFar = max(maxDistance, safeNear + 0.001f);
    float depth = clamp(viewDepth, safeNear, safeFar);

    return saturate(log(depth / safeNear) / log(safeFar / safeNear));
}

float4 SampleIntegratedFog(float2 uv, float normalizedDepth)
{
    uint gridDepth = max((uint) fogGridParams.x, 1u);
    float depthInSlices = normalizedDepth * float(gridDepth);

    if (depthInSlices <= 1.0f)
    {
        float firstSliceZ = 0.5f / float(gridDepth);

        float4 firstSlice = integratedFogVolume.SampleLevel(linearClampSample, float3(uv, firstSliceZ), 0.0f);

        return lerp(float4(0.0f, 0.0f, 0.0f, 1.0f), firstSlice, saturate(depthInSlices));
    }

    float volumeZ = (depthInSlices - 0.5f) / float(gridDepth);

    return integratedFogVolume.SampleLevel(linearClampSample, float3(uv, saturate(volumeZ)), 0.0f);
}

float4 main(PSInput input) : SV_Target
{
    int2 pixelCoord = int2(input.position.xy);
    float4 transparencyData = dynamicTransparencyMask.Load(int3(pixelCoord, 0));

    float depthBias = falloffSettings.x;
    float influence = EvaluateDynamicTransparencyInfluence(transparencyData, input.position.z, depthBias);

    bool hasDissolveComponent = falloffSettings.y > 0.5f;
    float dissolveAmount = falloffSettings.z;

    if (influence <= 0.0f)
        discard;
    if (influence >= DYNAMIC_TRANSPARENCY_CORE_THRESHOLD)
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
    float3 finalColor = ComputePBRSurfaceLighting(input.worldPos, albedo, metallic, alphaRoughness,ao, emissive, finalWorldNormal, 1.0f, input.position);

    bool fogEnabled = falloffSettings.w > 0.5f;

    if (fogEnabled)
    {
    // IMPORTANT:
    // input.position.z is the device depth of the transparent WALL fragment,
    // not MainDepth, which belongs to the real scene behind it.
        float wallViewDepth = LinearizeFogViewDepth(input.position.z);
        float normalizedFogDepth = GetNormalizedFogDepth(wallViewDepth);

        float2 fogUV = input.position.xy * invScreenSize;
        float4 integratedFog = SampleIntegratedFog(fogUV, normalizedFogDepth);

        finalColor = integratedFog.rgb + finalColor * saturate(integratedFog.a);
    }

    float wallOpacity = saturate(1.0f - influence);

    return float4(finalColor, wallOpacity);
}