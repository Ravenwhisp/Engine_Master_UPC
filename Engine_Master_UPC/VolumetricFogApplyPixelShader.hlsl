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
    uint padding;
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

float GetDebugSliceZ()
{
    float slice = round(saturate(debugSlice) * float(gridDepth - 1));
    return (slice + 0.5f) / float(gridDepth);
}

float3 DebugExposure(float3 value, float scale)
{
    return saturate(1.0f - exp(-max(value, 0.0f) * scale));
}

float4 main(float4 position : SV_Position, float2 coord : TEXCOORD0) : SV_TARGET
{
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

    if (debugView == DEBUG_SCATTERING)
        return float4(DebugExposure(integrated.rgb, 1.0f), 0.0f);
    if (debugView == DEBUG_TRANSMITTANCE)
        return float4(integrated.aaa, 0.0f);

    return float4(integrated.rgb, saturate(integrated.a));
}