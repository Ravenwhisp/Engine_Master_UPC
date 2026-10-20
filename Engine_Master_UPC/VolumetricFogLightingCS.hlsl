#include "VolumetricFogSampling.hlsli"
#include "VolumetricFogHeight.hlsli"
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

float FilterFogShadowProjection(float3 ndc, uint index, float2 offset)
{
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
    float depth = ndc.z - shadowBias;
    float visibility = 0.25f * (
        CompareCascade(index, uv + float2(-offset.x, -offset.y), depth) +
        CompareCascade(index, uv + float2( offset.x, -offset.y), depth) +
        CompareCascade(index, uv + float2(-offset.x,  offset.y), depth) +
        CompareCascade(index, uv + float2( offset.x,  offset.y), depth));
    return lerp(1.0f - shadowStrength, 1.0f, visibility);
}

// Cache only when a convex clip-volume test proves the entire segment uses
// the same first covering cascade. Endpoints choosing the same cascade alone
// would NOT exclude an earlier cascade intersecting the segment's interior.
bool PrepareFogShadowSegment(float3 startPosition, float3 endPosition,
    out uint selectedCascade, out float4 projectedStart, out float4 projectedEnd,
    out float2 filterOffset)
{
    selectedCascade = MAX_SHADOW_CASCADES;
    projectedStart = projectedEnd = 0.0f;
    filterOffset = 0.0f;
    uint count = clamp(cascadeCount, 1u, (uint)MAX_SHADOW_CASCADES);
    [loop]
    for (uint index = 0u; index < count; ++index)
    {
        float4 a = mul(float4(startPosition, 1.0f), cascadeLightViewProjection[index]);
        float4 b = mul(float4(endPosition, 1.0f), cascadeLightViewProjection[index]);
        if (a.w <= 0.0f || b.w <= 0.0f)
            return false;

        float3 na = a.xyz / a.w;
        float3 nb = b.xyz / b.w;
        float3 low = min(na, nb);
        float3 high = max(na, nb);
        // Conservative margin: boundary cases retain the original per-sample path.
        const float margin = 0.0001f;
        bool outside = high.x < -1.0f - margin || low.x > 1.0f + margin ||
                       high.y < -1.0f - margin || low.y > 1.0f + margin ||
                       high.z < -margin || low.z > 1.0f + margin;
        if (outside)
            continue;

        bool inside = all(low > float3(-1.0f + margin, -1.0f + margin, margin)) &&
                      all(high < float3(1.0f - margin, 1.0f - margin, 1.0f - margin));
        if (!inside)
            return false;

        selectedCascade = index;
        projectedStart = a;
        projectedEnd = b;
        filterOffset = GetCascadeTexelSize(index) * ((float(pcfRadius) + 0.5f) * 0.5f);
        return true;
    }
    // Every cascade excludes the complete segment: all queries would return lit.
    return true;
}

// Fog-only approximation. Keep the original cascade selection, coverage test,
// comparison depth, sampler and strength; reduce only the number of PCF taps.
float ComputeVolumetricShadow(float3 worldPosition, out uint selectedCascadeIndex)
{
    selectedCascadeIndex = MAX_SHADOW_CASCADES;
    if (shadowsEnabled == 0u)
        return 1.0f;

    // Retain the exact original path for A/B and for single-comparison shadows.
    if (optimizedShadowFilterEnabled == 0u || pcfEnabled == 0u || pcfRadius == 0u)
    {
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
        selectedCascadeIndex = index;

        // One bilinear comparison at the center of each quadrant of the
        // original square footprint. This approximates, not reproduces, box PCF.
        float2 offset = GetCascadeTexelSize(index) * ((float(pcfRadius) + 0.5f) * 0.5f);
        return FilterFogShadowProjection(ndc, index, offset);
    }
    return 1.0f;
}

float ComputeVolumetricShadow(float3 worldPosition)
{
    uint cascadeIndex;
    return ComputeVolumetricShadow(worldPosition, cascadeIndex);
}

float ComputeFogSegmentShadow(uint3 froxel, uint3 gridSize, float extinction)
{
    if (shadowsEnabled == 0u || shadowStrength == 0.0f)
        return 1.0f;

    // Use shared slice boundaries, independent of density jitter. Adjacent
    // slices therefore agree on visibility at their common boundary.
    float3 startPosition = GetFroxelWorldPositionJittered(
        froxel, gridSize, projectionScale, nearDistance, maxDistance, inverseView, 0.0f);
    float3 endPosition = GetFroxelWorldPositionJittered(
        froxel, gridSize, projectionScale, nearDistance, maxDistance, inverseView, 1.0f);
    if (shadowDiagnostic == 6u || heightFogDensity > 0.0f)
    {
        // The medium stores the exact height-profile average over this segment.
        // Redistribute its optical depth over subsegments using the same profile.
        // Its existing animation multiplier is retained, not resampled or delayed.
        float tau = max(extinction, 0.0f) * length(endPosition - startPosition);
        uint sampleCount = clamp(shadowSampleCount, 1u, 32u);
        float inverseSampleCount = rcp(float(sampleCount));
        float sum = 0.0f;
        float weights = 0.0f;
        float transmission = 1.0f;
        float averageDensity = uniformFogDensity + heightFogDensity * FogHeightAverage(
            startPosition.y, endPosition.y, heightFogBaseHeight, heightFogFalloff);
        if (tau <= 0.0f || averageDensity <= 0.0f)
            return 1.0f;
        uint segmentCascade = MAX_SHADOW_CASCADES;
        float4 projectedStart = 0.0f;
        float4 projectedEnd = 0.0f;
        float2 filterOffset = 0.0f;
        bool reuseProjection = false;
        // Amortize setup only for multiple samples with the accepted four-tap filter.
        if (sampleCount >= 4u && optimizedShadowFilterEnabled != 0u &&
            pcfEnabled != 0u && pcfRadius != 0u)
        {
            reuseProjection = PrepareFogShadowSegment(startPosition, endPosition,
                segmentCascade, projectedStart, projectedEnd, filterOffset);
            if (reuseProjection && segmentCascade == MAX_SHADOW_CASCADES)
                return 1.0f;
        }
        [loop]
        for (uint i = 0u; i < sampleCount; ++i)
        {
            float t = (float(i) + 0.5f) * inverseSampleCount;
            float y0 = lerp(startPosition.y, endPosition.y, float(i) * inverseSampleCount);
            float y1 = lerp(startPosition.y, endPosition.y, float(i + 1u) * inverseSampleCount);
            float localDensity = uniformFogDensity + heightFogDensity * FogHeightAverage(
                y0, y1, heightFogBaseHeight, heightFogFalloff);
            float localTau = tau * inverseSampleCount * (localDensity / averageDensity);
            // Share the exponential between absorption and transmission. Retain
            // the original small-depth series to avoid cancellation.
            float x = max(localTau, 0.0f);
            float stepTransmission = exp(-x);
            float absorption = x < 0.01f
                ? x * (1.0f - x * 0.5f + x * x / 6.0f)
                : 1.0f - stepTransmission;
            float weight = transmission * absorption;
            float visibility;
            if (reuseProjection)
            {
                // Matrix multiplication is linear: interpolate in homogeneous
                // space, then divide by w, preserving the original sample locations.
                float4 projected = lerp(projectedStart, projectedEnd, t);
                visibility = FilterFogShadowProjection(
                    projected.xyz / projected.w, segmentCascade, filterOffset);
            }
            else
            {
                visibility = ComputeVolumetricShadow(lerp(startPosition, endPosition, t));
            }
            sum += weight * visibility;
            weights += weight;
            transmission *= stepTransmission;
        }
        return weights > 0.0f ? sum / weights : 1.0f;
    }
    float startVisibility = ComputeVolumetricShadow(startPosition);
    float endVisibility = ComputeVolumetricShadow(endPosition);

    // Integrate a linear visibility transition against Beer-Lambert attenuation.
    // IntegrateCS subsequently multiplies this average by the segment integral.
    // This is a reconstruction between two samples, not an exact shadow-volume
    // intersection: shadow features smaller than a slice can still be missed.
    float opticalDepth = max(extinction, 0.0f) * length(endPosition - startPosition);
    float startWeight;
    if (opticalDepth < 0.05f)
    {
        // Series avoids cancellation near zero, including a vacuum.
        startWeight = 0.5f + opticalDepth / 12.0f
            - opticalDepth * opticalDepth * opticalDepth / 720.0f;
    }
    else
    {
        float transmission = exp(-opticalDepth);
        startWeight = 1.0f - rcp(opticalDepth)
            + transmission / (1.0f - transmission);
    }
    return lerp(endVisibility, startVisibility, saturate(startWeight));
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

    if (shadowDiagnostic == 7u || shadowDiagnostic == 8u)
    {
        float3 diagnostic = float3(1.0f, 0.0f, 1.0f);
        // Do not access shadow resources when the pass has no valid bindings.
        if (hasDirectionalLight != 0u && debugDisableShadows == 0u)
        {
            float3 center = GetFroxelWorldPosition(
                dispatchThreadID, gridSize, projectionScale, nearDistance, maxDistance, inverseView);
            uint cascadeIndex;
            float visibility = ComputeVolumetricShadow(center, cascadeIndex);
            if (cascadeIndex < MAX_SHADOW_CASCADES)
            {
                float rawVisibility = shadowStrength > 0.000001f
                    ? saturate((visibility - (1.0f - shadowStrength)) / shadowStrength) : 1.0f;
                diagnostic = shadowDiagnostic == 7u
                    ? rawVisibility.xxx : GetCascadeDebugColor(cascadeIndex);
            }
        }
        lightingVolume[dispatchThreadID] = float4(diagnostic, 0.0f);
        return;
    }

    float sampleOffset = GetFogSampleOffset(dispatchThreadID, samplingJitterEnabled, samplingJitterStrength);

    float4 medium = mediumVolume.Load(int4(dispatchThreadID, 0));
    // Exactly zero scattering cannot contribute light. Extinction remains in
    // mediumVolume and is still integrated normally; diagnostic slices run above.
    if (all(medium.rgb == 0.0f))
    {
        lightingVolume[dispatchThreadID] = 0.0f;
        return;
    }

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
            shadow = ComputeFogSegmentShadow(dispatchThreadID, gridSize, medium.a);
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
