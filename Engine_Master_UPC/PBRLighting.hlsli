#ifndef PBR_LIGHTING_HLSLI
#define PBR_LIGHTING_HLSLI

#include "LightingCBuffers.hlsli"
#include "General.hlsli"
#include "PBRGeneral.hlsli"
#include "LightTileCullingCommon.hlsli"

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);
Texture2DArray shadowMap : register(t11);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);


// ---------- DIRECT LIGHTING ----------

float3 LightCalculation(float3 lightDirection, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 diffuseColor, float3 lightColor, float3 F0)
{
    float3 halfVector = normalize(lightDirection + viewDirection);

    float NdotL = clamp(-dot(normalVector, lightDirection), 0.001, 1.0);
    float NdotH = saturate(dot(normalVector, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));

    float3 fresnel = SchlickFresnel(F0, NdotH);
    float smithVisibility = SmithVisibilityFunction(NdotL, NdotV, alphaRoughness);
    float normalDistribution = NormalDistributionFunction(NdotH, alphaRoughness);

    return (diffuseColor + (0.25 * fresnel * smithVisibility * normalDistribution)) * lightColor * NdotL;
}

float3 ComputeDirectionalLight(uint lightIndex, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 lightDirection = normalize(directionalLights[lightIndex].direction);
    float3 lightColor = directionalLights[lightIndex].color * directionalLights[lightIndex].intensity;

    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
}

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= EPS)
        return 0.0f;

    float normalizedDistance = distanceValue / radiusValue;
    float normalizedDistance2 = normalizedDistance * normalizedDistance;
    float normalizedDistance4 = normalizedDistance2 * normalizedDistance2;

    float numerator = max(1.0f - normalizedDistance4, 0.0f);
    numerator *= numerator;

    float denominator = distanceValue * distanceValue + 1.0f;

    return numerator / denominator;
}

float3 ComputePointLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 toSurface = worldPos - pointLights[lightIndex].position;
    float distanceToSurface = length(toSurface);

    if (distanceToSurface <= EPS)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceToSurface, pointLights[lightIndex].radius);

    float3 lightDirection = toSurface / distanceToSurface;
    float3 lightColor = pointLights[lightIndex].color * pointLights[lightIndex].intensity * attenuation;

    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, EPS);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 ComputeSpotLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 spotDirection = normalize(spotLights[lightIndex].direction);
    float3 toSurface = worldPos - spotLights[lightIndex].position;

    float distanceProjected = dot(toSurface, spotDirection);

    if (distanceProjected <= 0.0f)
        return 0.0f;

    float3 lightDirection = normalize(toSurface);

    float attenuation = EpicAttenuation(distanceProjected, spotLights[lightIndex].radius);
    float cosineAngle = dot(lightDirection, spotDirection);
    float coneAttenuation = SpotConeAttenuation(cosineAngle, spotLights[lightIndex].cosineInnerAngle, spotLights[lightIndex].cosineOuterAngle);

    float3 lightColor = spotLights[lightIndex].color * spotLights[lightIndex].intensity * attenuation * coneAttenuation;

    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
}


// ---------- INDIRECT LIGHTING ----------

float computeSpecularAO(float NdotV, float diffuseAO, float roughness)
{
    return saturate(pow(NdotV + diffuseAO, exp2(-16.0 * roughness - 1.0)) - 1.0 + diffuseAO);
}

float3 getDiffuseAmbientLight(float3 normal, float3 baseColour)
{
    float3 irradiance = irradianceTexture.SampleLevel(linearWrapSample, normal, 0).rgb;
    return baseColour * irradiance;
}

void getSpecularAmbientLightNoFresnel(float3 R, float NdotV, float roughness, uint numLevels, out float3 firstTerm, out float3 secondTerm)
{
    float3 radiance = environmentTexture.SampleLevel(linearWrapSample, R, roughness * (numLevels - 1)).rgb;
    float2 fab = brdfTexture.Sample(linearClampSample, float2(NdotV, roughness)).rg;

    firstTerm = radiance * fab.x;
    secondTerm = radiance * fab.y;
}

float3 computeIndirectLighting(float3 R, float NdotV, float3 N, float3 baseColour, float roughness, float roughnessLevels, float metallic, float ao, float specularAO)
{
    float3 diffuse = getDiffuseAmbientLight(N, baseColour);
    diffuse *= ao;

    float3 firstTerm;
    float3 secondTerm;

    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, firstTerm, secondTerm);

    float3 metalSpecular = baseColour * firstTerm + secondTerm;
    metalSpecular *= specularAO;

    float3 dielectricSpecular = DIELECTRIC_FRESNEL * firstTerm + secondTerm;
    dielectricSpecular *= specularAO;

    return lerp(diffuse + dielectricSpecular, metalSpecular, metallic);
}


// ---------- SHADOW MAPPING ----------

float4x4 GetCascadeViewProjection(uint cascadeIndex)
{
    if (cascadeIndex == 0)
        return cascadeLightViewProjection[0];

    if (cascadeIndex == 1)
        return cascadeLightViewProjection[1];

    if (cascadeIndex == 2)
        return cascadeLightViewProjection[2];

    return cascadeLightViewProjection[3];
}

bool GetCascadeShadowCoordinates(float3 worldPos, uint cascadeIndex, out float2 shadowUV, out float currentDepth)
{
    shadowUV = float2(0.0f, 0.0f);
    currentDepth = 0.0f;

    float4 shadowPos = mul(float4(worldPos, 1.0f), GetCascadeViewProjection(cascadeIndex));

    if (shadowPos.w == 0.0f)
        return false;

    shadowPos.xyz /= shadowPos.w;

    shadowUV.x = shadowPos.x * 0.5f + 0.5f;
    shadowUV.y = -shadowPos.y * 0.5f + 0.5f;
    currentDepth = shadowPos.z;

    return shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
           shadowUV.y >= 0.0f && shadowUV.y <= 1.0f &&
           currentDepth >= 0.0f && currentDepth <= 1.0f;
}

float EvaluateShadowSample(uint cascadeIndex, float2 shadowUV, float currentDepth)
{
    float closestDepth = shadowMap.Sample(linearClampSample, float3(shadowUV, float(cascadeIndex))).r;
    return currentDepth - shadowBias > closestDepth ? 1.0f - shadowStrength : 1.0f;
}

float ComputeCascadeShadow(uint cascadeIndex, float2 shadowUV, float currentDepth)
{
    if (pcfEnabled == 0 || pcfRadius == 0)
        return EvaluateShadowSample(cascadeIndex, shadowUV, currentDepth);

    float shadowSum = 0.0f;
    float sampleCount = 0.0f;
    int radius = int(pcfRadius);

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            float2 offset = float2(x, y) * shadowMapTexelSize;
            float2 sampleUV = shadowUV + offset;

            if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
                shadowSum += 1.0f;
            else
                shadowSum += EvaluateShadowSample(cascadeIndex, sampleUV, currentDepth);

            sampleCount += 1.0f;
        }
    }

    return shadowSum / sampleCount;
}

float ComputeShadow(float3 worldPos, out uint selectedCascadeIndex)
{
    selectedCascadeIndex = MAX_SHADOW_CASCADES;

    if (shadowsEnabled == 0)
        return 1.0f;

    uint activeCascadeCount = clamp(cascadeCount, 1u, (uint) MAX_SHADOW_CASCADES);

    float2 shadowUV;
    float currentDepth;

    if (GetCascadeShadowCoordinates(worldPos, 0u, shadowUV, currentDepth))
    {
        selectedCascadeIndex = 0u;
        return ComputeCascadeShadow(0u, shadowUV, currentDepth);
    }

    if (activeCascadeCount > 1u && GetCascadeShadowCoordinates(worldPos, 1u, shadowUV, currentDepth))
    {
        selectedCascadeIndex = 1u;
        return ComputeCascadeShadow(1u, shadowUV, currentDepth);
    }

    if (activeCascadeCount > 2u && GetCascadeShadowCoordinates(worldPos, 2u, shadowUV, currentDepth))
    {
        selectedCascadeIndex = 2u;
        return ComputeCascadeShadow(2u, shadowUV, currentDepth);
    }

    if (activeCascadeCount > 3u && GetCascadeShadowCoordinates(worldPos, 3u, shadowUV, currentDepth))
    {
        selectedCascadeIndex = 3u;
        return ComputeCascadeShadow(3u, shadowUV, currentDepth);
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

// directional stays untiled (only 4 max); otherMetallic/otherNonMetallic come pre-accumulated
float3 ComputePBRSurfaceLightingCommon(float3 worldPos, float3 albedo, float metallic, float alphaRoughness, float ao, float3 emissive, float3 finalWorldNormal, float screenSpaceAO,
    float3 F0Metallic, float3 F0NonMetallic, float3 viewDirection, float NdotV, float horizon, float3 otherMetallic, float3 otherNonMetallic)
{
    float3 directionalMetallic = 0.0f;
    float3 directionalNonMetallic = 0.0f;

    float3 diffuseColorMetallic = 0.0f;
    float3 diffuseColorNonMetallic = albedo / PI;

    for (uint i = 0; i < directionalCount; ++i)
    {
        directionalMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        directionalNonMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    uint selectedCascadeIndex;
    float shadow = ComputeShadow(worldPos, selectedCascadeIndex);

    float3 directionalLighting = lerp(directionalNonMetallic, directionalMetallic, metallic);
    float3 otherLighting = lerp(otherNonMetallic, otherMetallic, metallic);
    float3 directLighting = directionalLighting * shadow + otherLighting;

    float diffuseAO = saturate(ao * screenSpaceAO);
    float specularAO = computeSpecularAO(NdotV, diffuseAO, alphaRoughness);
    specularAO *= horizon;

    float3 reflection = normalize(reflect(-viewDirection, finalWorldNormal));

    float3 indirectLighting = computeIndirectLighting(reflection, NdotV, finalWorldNormal, F0Metallic, alphaRoughness, 11, metallic, diffuseAO, specularAO);
    float3 finalColor = directLighting + indirectLighting + emissive;

    if (cascadePadding.x > 0.5f && selectedCascadeIndex < MAX_SHADOW_CASCADES)
        finalColor = lerp(finalColor, GetCascadeDebugColor(selectedCascadeIndex), 0.35f);

    return finalColor;
}

// walks the per-tile light lists LightCullingPass built - used by every pixel shader now
float3 ComputePBRSurfaceLightingTiled(float3 worldPos, float3 albedo, float metallic, float alphaRoughness, float ao, float3 emissive, float3 finalWorldNormal, float screenSpaceAO,
    uint tileIndex, StructuredBuffer<int> pointLightIndices, StructuredBuffer<int> spotLightIndices)
{
    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04f;

    float3 diffuseColorMetallic = 0.0f;
    float3 diffuseColorNonMetallic = albedo / PI;

    float3 viewDirection = normalize(viewPos - worldPos);
    float3 reflection = normalize(reflect(-viewDirection, finalWorldNormal));
    float NdotV = abs(dot(finalWorldNormal, viewDirection)) + 0.001f;
    float horizon = min(1.0f + dot(reflection, finalWorldNormal), 1.0f);

    alphaRoughness *= alphaRoughness;

    float3 otherMetallic = 0.0f;
    float3 otherNonMetallic = 0.0f;

    const uint tileBase = tileIndex * MAX_LIGHTS_PER_TILE;

    for (uint p = 0; p < MAX_LIGHTS_PER_TILE; ++p)
    {
        int lightIndex = pointLightIndices[tileBase + p];

        if (lightIndex < 0)
        {
            break;
        }

        otherMetallic += ComputePointLight((uint) lightIndex, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputePointLight((uint) lightIndex, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    for (uint s = 0; s < MAX_LIGHTS_PER_TILE; ++s)
    {
        int lightIndex = spotLightIndices[tileBase + s];

        if (lightIndex < 0)
        {
            break;
        }

        otherMetallic += ComputeSpotLight((uint) lightIndex, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputeSpotLight((uint) lightIndex, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    return ComputePBRSurfaceLightingCommon(worldPos, albedo, metallic, alphaRoughness, ao, emissive, finalWorldNormal, screenSpaceAO,
        F0Metallic, F0NonMetallic, viewDirection, NdotV, horizon, otherMetallic, otherNonMetallic);
}

#endif