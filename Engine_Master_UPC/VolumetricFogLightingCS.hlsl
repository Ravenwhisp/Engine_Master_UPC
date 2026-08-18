#include "VolumetricFogCommon.hlsli"
#define MAX_SHADOW_CASCADES 4

cbuffer ShadowData : register(b1)
{
    float4x4 lightViewProjection;

    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;

    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;

    uint cascadeCount;
    uint cascadeFitMode;
    float2 cascadePadding;

    float4 cascadeFarDistances;
    float4x4 cascadeLightViewProjection[MAX_SHADOW_CASCADES];
};

Texture2DArray<float> shadowMap : register(t1);
SamplerState shadowSampler : register(s0);

cbuffer LightingConstants : register(b0)
{
    float4x4 inverseView;

    float2 projectionScale;
    float nearDistance;
    float maxDistance;

    float3 cameraPosition;
    float anisotropy;

    float3 lightDirection;
    float lightIntensity;

    float3 lightColor;
    uint hasDirectionalLight;

    uint gridWidth;
    uint gridHeight;
    uint gridDepth;
    uint padding;
};

Texture3D<float4> mediumVolume : register(t0);
RWTexture3D<float4> lightingVolume : register(u0);

static const float PI = 3.14159265359f;

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denominator = max(1.0f + g2 - 2.0f * g * cosTheta, 0.0001f);
    return (1.0f - g2) / (4.0f * PI * pow(denominator, 1.5f));
}

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

bool GetCascadeShadowCoordinates(float3 worldPosition, uint cascadeIndex, out float2 shadowUV, out float currentDepth)
{
    shadowUV = 0.0f;
    currentDepth = 0.0f;

    float4 shadowPosition = mul(float4(worldPosition, 1.0f), GetCascadeViewProjection(cascadeIndex));
    if (shadowPosition.w == 0.0f)
        return false;

    shadowPosition.xyz /= shadowPosition.w;

    shadowUV = float2(shadowPosition.x * 0.5f + 0.5f, -shadowPosition.y * 0.5f + 0.5f);
    currentDepth = shadowPosition.z;

    return shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f && currentDepth >= 0.0f && currentDepth <= 1.0f;
}

float EvaluateShadowSample(uint cascadeIndex, float2 shadowUV, float currentDepth)
{
    float closestDepth = shadowMap.SampleLevel(shadowSampler, float3(shadowUV, float(cascadeIndex)), 0.0f).r;
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
            float2 sampleUV = shadowUV + float2(x, y) * shadowMapTexelSize;
            shadowSum += sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f ? 1.0f : EvaluateShadowSample(cascadeIndex, sampleUV, currentDepth);
            sampleCount += 1.0f;
        }
    }

    return shadowSum / sampleCount;
}

float ComputeShadow(float3 worldPosition)
{
    if (shadowsEnabled == 0)
        return 1.0f;

    uint activeCascadeCount = clamp(cascadeCount, 1u, (uint) MAX_SHADOW_CASCADES);
    float2 shadowUV;
    float currentDepth;

    if (GetCascadeShadowCoordinates(worldPosition, 0u, shadowUV, currentDepth))
        return ComputeCascadeShadow(0u, shadowUV, currentDepth);
    if (activeCascadeCount > 1u && GetCascadeShadowCoordinates(worldPosition, 1u, shadowUV, currentDepth))
        return ComputeCascadeShadow(1u, shadowUV, currentDepth);
    if (activeCascadeCount > 2u && GetCascadeShadowCoordinates(worldPosition, 2u, shadowUV, currentDepth))
        return ComputeCascadeShadow(2u, shadowUV, currentDepth);
    if (activeCascadeCount > 3u && GetCascadeShadowCoordinates(worldPosition, 3u, shadowUV, currentDepth))
        return ComputeCascadeShadow(3u, shadowUV, currentDepth);

    return 1.0f;
}

[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight || dispatchThreadID.z >= gridDepth)
        return;

    if (hasDirectionalLight == 0)
    {
        lightingVolume[dispatchThreadID] = 0.0f;
        return;
    }

    uint3 gridSize = uint3(gridWidth, gridHeight, gridDepth);
    float4 medium = mediumVolume.Load(int4(dispatchThreadID, 0));
    float3 worldPosition = GetFroxelWorldPosition(dispatchThreadID, gridSize, projectionScale, nearDistance, maxDistance, inverseView);
    float3 viewDirection = normalize(cameraPosition - worldPosition);
    float3 incomingDirection = normalize(lightDirection);
    float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);
    float shadow = ComputeShadow(worldPosition);
    float3 inScattering = medium.rgb * lightColor * lightIntensity * phase * shadow;

    lightingVolume[dispatchThreadID] = float4(inScattering, 0.0f);
}