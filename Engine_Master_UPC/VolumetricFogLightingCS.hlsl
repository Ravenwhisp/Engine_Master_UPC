#include "VolumetricFogCommon.hlsli"
#define MAX_SHADOW_CASCADES 4

#define SHADOW_CB_REGISTER b1
#include "CascadeShadowMaps.hlsli"

#include "VolumetricFogLightingData.hlsli"

#define MAX_SPOT_LIGHTS 64

struct SpotLight
{
    float3 position;
    float radius;

    float3 direction;
    float padding0;

    float3 color;
    float intensity;

    float cosineInnerAngle;
    float cosineOuterAngle;
    float2 padding1;
};

cbuffer SpotLightsCB : register(b3)
{
    uint spotLightCount;
    uint3 spotLightPadding;

    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

Texture3D<float4> mediumVolume : register(t0);
StructuredBuffer<uint> blockLightMasks : register(t1);
RWTexture3D<float4> lightingVolume : register(u0);

static const float PI = 3.14159265359f;

// Fog-only approximation. Keep the original cascade selection, coverage test,
// comparison depth, sampler and strength; reduce only the number of PCF taps.
float ComputeVolumetricShadow(float3 worldPosition)
{
    if (shadowsEnabled == 0u)
        return 1.0f;

    // Retain the exact original path for A/B and for single-comparison shadows.
    if (optimizedShadowFilterEnabled == 0u || pcfEnabled == 0u || pcfRadius == 0u)
    {
        uint selectedCascadeIndex;
        return ComputeShadow(worldPosition, float3(0.0f, 0.0f, 0.0f), selectedCascadeIndex);
    }

    uint count = clamp(cascadeCount, 1u, (uint) MAX_SHADOW_CASCADES);
    [loop]
    for (uint index = 0u; index < count; ++index)
    {
        float4 projected = mul(float4(worldPosition, 1.0f), cascadeLightViewProjection[index]);
        float3 ndc = projected.xyz / projected.w;
        float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f)
            continue;

        // One bilinear comparison at the center of each quadrant of the
        // original square footprint. This approximates, not reproduces, box PCF.
        float2 offset = GetCascadeTexelSize(index) * ((float(pcfRadius) + 0.5f) * 0.5f);
        float depth = ndc.z - shadowBias;
        float visibility = 0.25f * (
            CompareCascade(index, uv + float2(-offset.x, -offset.y), depth) +
            CompareCascade(index, uv + float2( offset.x, -offset.y), depth) +
            CompareCascade(index, uv + float2(-offset.x,  offset.y), depth) +
            CompareCascade(index, uv + float2( offset.x,  offset.y), depth));
        return lerp(1.0f - shadowStrength, 1.0f, visibility);
    }
    return 1.0f;
}

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denominator = max(1.0f + g2 - 2.0f * g * cosTheta, 0.0001f);
    return (1.0f - g2) / (4.0f * PI * pow(denominator, 1.5f));
}

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= 0.000001f)
        return 0.0f;

    float normalizedDistance = distanceValue / radiusValue;
    float normalizedDistance2 = normalizedDistance * normalizedDistance;
    float normalizedDistance4 = normalizedDistance2 * normalizedDistance2;

    float numerator = max(1.0f - normalizedDistance4, 0.0f);
    numerator *= numerator;

    float denominator = distanceValue * distanceValue + 1.0f;

    return numerator / denominator;
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, 0.000001f);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 ComputePointVolumetricScattering(PointLight light, float3 worldPosition, float3 viewDirection, float3 mediumScattering)
{
    if (light.radius <= 0.000001f)
        return 0.0f;

    float3 toFroxel = worldPosition - light.position;
    float distanceToFroxel = length(toFroxel);

    if (distanceToFroxel <= 0.000001f || distanceToFroxel >= light.radius)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceToFroxel, light.radius);

    if (attenuation <= 0.0f)
        return 0.0f;

    float3 incomingDirection = toFroxel / distanceToFroxel;
    float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

    return mediumScattering * light.color * light.intensity * attenuation * phase;
}

float3 ComputeSpotVolumetricScattering(SpotLight light, float3 worldPosition, float3 viewDirection, float3 mediumScattering)
{
    if (light.radius <= 0.000001f)
        return 0.0f;

    float3 spotDirection = normalize(light.direction);
    float3 toFroxel = worldPosition - light.position;

    float distanceProjected = dot(toFroxel, spotDirection);

    if (distanceProjected <= 0.0f || distanceProjected >= light.radius)
        return 0.0f;

    float distanceToFroxel = length(toFroxel);

    if (distanceToFroxel <= 0.000001f)
        return 0.0f;

    float3 incomingDirection = toFroxel / distanceToFroxel;

    float cosineAngle = dot(incomingDirection, spotDirection);

    if (cosineAngle <= light.cosineOuterAngle)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceProjected, light.radius);

    if (attenuation <= 0.0f)
        return 0.0f;

    float coneAttenuation = SpotConeAttenuation(cosineAngle, light.cosineInnerAngle, light.cosineOuterAngle);

    if (coneAttenuation <= 0.0f)
        return 0.0f;

    float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

    return mediumScattering * light.color * light.intensity * attenuation * coneAttenuation * phase;
}


[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight || dispatchThreadID.z >= gridDepth)
        return;

    uint3 gridSize = uint3(gridWidth, gridHeight, gridDepth);

    float sampleOffset = GetFroxelSampleOffset(dispatchThreadID.xy, samplingJitterEnabled, samplingJitterStrength);

    float4 medium = mediumVolume.Load(int4(dispatchThreadID, 0));

    float3 worldPosition = GetFroxelWorldPositionJittered(
    dispatchThreadID,
    gridSize,
    projectionScale,
    nearDistance,
    maxDistance,
    inverseView,
    sampleOffset);

    float3 viewDirection = normalize(cameraPosition - worldPosition);

    float3 inScattering = 0.0f;

    if (hasDirectionalLight != 0)
    {
        float3 incomingDirection = normalize(lightDirection);
        float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

        float shadow = 1.0f;

        if (debugDisableShadows == 0)
        {
            shadow = ComputeVolumetricShadow(worldPosition);
        }

        inScattering += medium.rgb * lightColor * lightIntensity * phase * shadow;
    }

    uint activePointLightCount = min(pointLightCount, (uint) MAX_POINT_LIGHTS);

    if (pointLightCullingEnabled != 0u)
    {
        uint3 block = dispatchThreadID / uint3(FOG_LIGHT_BLOCK_X, FOG_LIGHT_BLOCK_Y, FOG_LIGHT_BLOCK_Z);
        uint maskBase = GetFogLightBlockIndex(block) * FOG_POINT_LIGHT_MASK_WORDS;
        // Ascending indices preserve the original order of floating-point additions.
        for (uint word = 0u; word < (activePointLightCount + 31u) / 32u; ++word)
        {
            uint mask = blockLightMasks[maskBase + word];
            [loop]
            while (mask != 0u)
            {
                uint pointIndex = word * 32u + (uint) firstbitlow(mask);
                mask &= mask - 1u;
                inScattering += ComputePointVolumetricScattering(pointLights[pointIndex], worldPosition, viewDirection, medium.rgb);
            }
        }
    }
    else
    {
        [loop]
        for (uint pointIndex = 0; pointIndex < activePointLightCount; ++pointIndex)
            inScattering += ComputePointVolumetricScattering(pointLights[pointIndex], worldPosition, viewDirection, medium.rgb);
    }
    
    uint activeSpotLightCount = min(spotLightCount, (uint) MAX_SPOT_LIGHTS);

    [loop]
    for (uint spotIndex = 0; spotIndex < activeSpotLightCount; ++spotIndex)
    {
        inScattering += ComputeSpotVolumetricScattering(spotLights[spotIndex], worldPosition, viewDirection, medium.rgb);
    }

    lightingVolume[dispatchThreadID] = float4(inScattering, 0.0f);
}
