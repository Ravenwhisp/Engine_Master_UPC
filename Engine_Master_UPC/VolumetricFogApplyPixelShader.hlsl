#include "VolumetricFogCommon.hlsli"

cbuffer ApplyConstants : register(b0)
{
    float nearDistance;
    float maxDistance;
    float projectionA;
    float projectionB;

    uint gridDepth;
    uint debugView;
    float debugSlice;
    uint padding0;

    uint distanceFogEnabled;
    float distanceFogStart;
    float distanceFogEnd;
    float distanceFogMaxOpacity;

    float distanceFogColorR;
    float distanceFogColorG;
    float distanceFogColorB;
    uint padding1;

    uint spatialSmoothingEnabled;
    float spatialSmoothingStrength;
    float inverseGridWidth;
    float inverseGridHeight;
};

Texture3D<float4> mediumVolume : register(t0);
Texture3D<float4> lightingVolume : register(t1);
Texture3D<float4> integratedVolume : register(t2);
Texture2D<float> depthTexture : register(t3);

SamplerState linearClampSampler : register(s0);

static const uint DEBUG_FINAL = 0;
static const uint DEBUG_MEDIUM = 1;
static const uint DEBUG_LIGHTING = 2;
static const uint DEBUG_LIGHTING_NO_SHADOWS = 3;
static const uint DEBUG_SCATTERING = 4;
static const uint DEBUG_TRANSMITTANCE = 5;

float4 SampleIntegratedVolume(float2 uv, float normalizedDepth)
{
    float depthInSlices = normalizedDepth * float(gridDepth);

    if (depthInSlices <= 1.0f)
    {
        float firstSliceZ = 0.5f / float(gridDepth);
        float4 firstSlice = integratedVolume.SampleLevel(linearClampSampler, float3(uv, firstSliceZ), 0.0f);

        return lerp(float4(0.0f, 0.0f, 0.0f, 1.0f), firstSlice, saturate(depthInSlices));
    }

    float volumeZ = (depthInSlices - 0.5f) / float(gridDepth);

    return integratedVolume.SampleLevel(linearClampSampler, float3(uv, saturate(volumeZ)), 0.0f);
}

float4 SmoothIntegratedFog(float2 uv, float viewDepth, float normalizedDepth, float4 center)
{
    if (spatialSmoothingEnabled == 0 || spatialSmoothingStrength <= 0.0f)
        return center;

    uint width, height;
    depthTexture.GetDimensions(width, height);
    // One froxel footprint, rather than one output pixel: the noise lives on
    // the low-resolution volume. Binomial 3x3 weights retain the center.
    float2 stepUV = float2(inverseGridWidth, inverseGridHeight);
    float4 sum = center * 4.0f;
    float totalWeight = 4.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            if (x == 0 && y == 0) continue;
            float2 neighborUV = uv + float2(x, y) * stepUV;
            if (any(neighborUV < 0.0f) || any(neighborUV >= 1.0f)) continue;
            int2 pixel = min(int2(neighborUV * float2(width, height)), int2(width, height) - 1);
            float neighborDepth = LinearizeVolumetricViewDepth(
                depthTexture.Load(int3(pixel, 0)), projectionA, projectionB);
            float depthDifference = abs(neighborDepth - viewDepth);
            // Stop at silhouettes instead of spreading fog across foreground edges.
            float tolerance = max(0.05f, viewDepth * 0.01f);
            float edgeWeight = saturate(1.0f - depthDifference / tolerance);
            float weight = (x == 0 || y == 0 ? 2.0f : 1.0f) * edgeWeight * edgeWeight;
            // Same integration endpoint for every tap avoids mixing ray lengths.
            sum += SampleIntegratedVolume(neighborUV, normalizedDepth) * weight;
            totalWeight += weight;
        }
    }
    // Filter scattering and transmittance together; never sample scene color.
    return lerp(center, sum / totalWeight, saturate(spatialSmoothingStrength));
}

float GetDebugSliceZ()
{
    float slice = round(saturate(debugSlice) * float(gridDepth - 1));

    return (slice + 0.5f) / float(gridDepth);
}

float3 DebugExposure(float3 value, float scale)
{
    return saturate(1.0f - exp(-max(value, 0.0f) * scale));
}

float GetDistanceFogAmount(float viewDepth)
{
    if (distanceFogEnabled == 0)
        return 0.0f;

    float distanceRange = max(distanceFogEnd - distanceFogStart, 0.001f);
    float normalizedDistance = saturate((viewDepth - distanceFogStart) / distanceRange);

    float smoothDistance = normalizedDistance * normalizedDistance * (3.0f - 2.0f * normalizedDistance);

    return smoothDistance * saturate(distanceFogMaxOpacity);
}

float4 main(float4 position : SV_Position, float2 coord : TEXCOORD0) : SV_TARGET
{
    if (debugView == 7u || debugView == 8u)
    {
        uint width, height, depth;
        lightingVolume.GetDimensions(width, height, depth);
        uint2 xy = min(uint2(saturate(coord) * float2(width, height)), uint2(width - 1u, height - 1u));
        uint z = (uint)round(saturate(debugSlice) * float(depth - 1u));
        // No exposure, interpolation or smoothing can hide the raw boundaries.
        return float4(lightingVolume.Load(int4(xy, z, 0)).rgb, 0.0f);
    }
    if (debugView == DEBUG_MEDIUM)
    {
        float4 medium = mediumVolume.SampleLevel(linearClampSampler, float3(coord, GetDebugSliceZ()), 0.0f);
        float extinction = 1.0f - exp(-max(medium.a, 0.0f) * 50.0f);

        return float4(extinction.xxx, 0.0f);
    }

    if (debugView == DEBUG_LIGHTING || debugView == DEBUG_LIGHTING_NO_SHADOWS)
    {
        float3 lighting = lightingVolume.SampleLevel(linearClampSampler, float3(coord, GetDebugSliceZ()), 0.0f).rgb;

        return float4(DebugExposure(lighting, 20.0f), 0.0f);
    }

    float deviceDepth = depthTexture.Load(int3(int2(position.xy), 0));
    float viewDepth = LinearizeVolumetricViewDepth(deviceDepth, projectionA, projectionB);
    float normalizedDepth = GetNormalizedVolumetricDepth(viewDepth, nearDistance, maxDistance);

    float4 integrated = SampleIntegratedVolume(coord, normalizedDepth);
    integrated = SmoothIntegratedFog(coord, viewDepth, normalizedDepth, integrated);

    if (debugView == DEBUG_SCATTERING)
        return float4(DebugExposure(integrated.rgb, 1.0f), 0.0f);

    if (debugView == DEBUG_TRANSMITTANCE)
        return float4(integrated.aaa, 0.0f);

    float distanceFogAmount = GetDistanceFogAmount(viewDepth);
    float3 distanceFogColor = float3(distanceFogColorR, distanceFogColorG, distanceFogColorB);

    float volumetricWeight = 1.0f - distanceFogAmount;

    float3 finalScattering = integrated.rgb * volumetricWeight + distanceFogColor * distanceFogAmount;
    float finalTransmittance = integrated.a * volumetricWeight;

    return float4(finalScattering, saturate(finalTransmittance));
}
