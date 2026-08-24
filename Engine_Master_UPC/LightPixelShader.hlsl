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

    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04f;

    float3 diffuseColorMetallic = 0.0f;
    float3 diffuseColorNonMetallic = albedo / PI;

    float3 viewDirection = normalize(viewPos - worldPos);
    float3 reflection = normalize(reflect(-viewDirection, finalWorldNormal));
    float NdotV = abs(dot(finalWorldNormal, viewDirection)) + 0.001f;
    float horizon = min(1.0f + dot(reflection, finalWorldNormal), 1.0f);

    alphaRoughness = alphaRoughness * alphaRoughness;

    float3 directionalMetallic = 0.0f;
    float3 directionalNonMetallic = 0.0f;

    for (uint i = 0; i < directionalCount; ++i)
    {
        directionalMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        directionalNonMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    float3 otherMetallic = 0.0f;
    float3 otherNonMetallic = 0.0f;

    for (uint i = 0; i < pointCount; ++i)
    {
        otherMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    for (uint i = 0; i < spotCount; ++i)
    {
        otherMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    uint selectedCascadeIndex;
    float shadow = ComputeShadow(worldPos, selectedCascadeIndex);

    float3 directionalLighting = lerp(directionalNonMetallic, directionalMetallic, metallic);
    float3 otherLighting = lerp(otherNonMetallic, otherMetallic, metallic);
    float3 directLighting = directionalLighting * shadow + otherLighting;

    float ssao = SampleSSAO(position);

    if (renderFlags.y > 0.5f)
        return float4(ssao.xxx, 1.0f);

    float diffuseAO = saturate(ao * ssao);

    float specularAO = computeSpecularAO(NdotV, diffuseAO, alphaRoughness);
    specularAO *= horizon;

    float3 indirectLighting = computeIndirectLighting(reflection, NdotV, finalWorldNormal, F0Metallic, alphaRoughness, 11, metallic, diffuseAO, specularAO);

    float3 finalColor = directLighting + indirectLighting + emissive;

    if (cascadePadding.x > 0.5f && selectedCascadeIndex < MAX_SHADOW_CASCADES)
        finalColor = lerp(finalColor, GetCascadeDebugColor(selectedCascadeIndex), 0.35f);

    return float4(finalColor, 1.0f);
}