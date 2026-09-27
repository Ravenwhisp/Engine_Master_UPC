#ifndef PBR_LIGHTING_HLSLI
#define PBR_LIGHTING_HLSLI

#include "LightingCBuffers.hlsli"
#include "General.hlsli"
#include "PBRGeneral.hlsli"
#include "LightTileCullingCommon.hlsli"

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);
#include "CascadeShadowMaps.hlsli"

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

float3 ComputePBRSurfaceLightingCommon(float3 worldPos, float3 albedo, float metallic, float alphaRoughness, float ao, float3 emissive, float3 finalWorldNormal, float screenSpaceAO,
    float3 F0Metallic, float3 F0NonMetallic, float3 viewDirection, float NdotV, float horizon, float3 otherMetallic, float3 otherNonMetallic)
{
    float3 directionalMetallic = 0.0f;
    float3 directionalNonMetallic = 0.0f;

    float3 diffuseColorMetallic = 0.0f;
    float3 diffuseColorNonMetallic = albedo / PI;

    uint selectedCascadeIndex;
    float shadow = ComputeShadow(worldPos, finalWorldNormal, selectedCascadeIndex);
    for (uint i = 0; i < directionalCount; ++i)
    {
        float visibility = i == shadowLightIndex ? shadow : 1.0f;
        directionalMetallic += visibility * ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        directionalNonMetallic += visibility * ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }


    float3 directionalLighting = lerp(directionalNonMetallic, directionalMetallic, metallic);
    float3 otherLighting = lerp(otherNonMetallic, otherMetallic, metallic);
    float3 directLighting = directionalLighting + otherLighting;

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

float3 ComputePBRSurfaceLightingUnculled(float3 worldPos, float3 albedo, float metallic, float alphaRoughness, float ao, float3 emissive, float3 finalWorldNormal, float screenSpaceAO)
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

    // Deferred depth ranges exclude surfaces removed from the GBuffer. A
    // transparent foreground wall must evaluate its own local-light support.
    for (uint p = 0; p < min(pointCount, (uint) MAX_POINT_LIGHTS); ++p)
    {
        float3 delta = worldPos - pointLights[p].position;
        if (pointLights[p].radius <= 0.0f || dot(delta, delta) >= pointLights[p].radius * pointLights[p].radius)
            continue;
        otherMetallic += ComputePointLight(p, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputePointLight(p, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    for (uint s = 0; s < min(spotCount, (uint) MAX_SPOT_LIGHTS); ++s)
    {
        float3 delta = worldPos - spotLights[s].position;
        float distanceAlongCone = dot(delta, normalize(spotLights[s].direction));
        if (distanceAlongCone <= 0.0f || distanceAlongCone >= spotLights[s].radius)
            continue;
        otherMetallic += ComputeSpotLight(s, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputeSpotLight(s, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    return ComputePBRSurfaceLightingCommon(worldPos, albedo, metallic, alphaRoughness, ao, emissive, finalWorldNormal, screenSpaceAO,
        F0Metallic, F0NonMetallic, viewDirection, NdotV, horizon, otherMetallic, otherNonMetallic);
}

#endif