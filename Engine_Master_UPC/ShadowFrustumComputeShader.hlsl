#include "ShadowCascadeResolution.h"

Texture2D<float2> inputMinMax : register(t0);

#define MAX_SHADOW_CASCADES 4
#define CASCADE_FIT_TO_SCENE 0
#define CASCADE_FIT_TO_CASCADE 1

struct ShadowDataOutput
{
    // Full fitted frustum used by the existing shadow pipeline.
    float4x4 lightViewProjection;

    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;

    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;

    // CSM
    uint cascadeCount;
    uint cascadeFitMode;
    float2 cascadePadding;

    float4 cascadeFarDistances;

    float4x4 cascadeLightViewProjection[MAX_SHADOW_CASCADES];
    
    float4x4 shadowCameraView;
    float4 cascadeWorldTexelSize;
    float4 cascadeDepthRanges;
    float4 shadowLightDirection;

    uint shadowLightIndex;
    float cascadeBlendFraction;
    float normalBiasTexels;
    float slopeBiasTexels;
};

RWStructuredBuffer<ShadowDataOutput> outputShadowData : register(u0);

cbuffer ShadowFrustumParams : register(b0)
{
    float4x4 inverseView;
    float4x4 cameraView;
    float4x4 cameraProjection;

    float3 lightDirection;
    float sunDistance;

    float shadowBias;
    float shadowStrength;
    uint pcfEnabled;
    uint pcfRadius;

    float shadowMapTexelSize;
    uint cascadeCount;
    uint cascadeFitMode;
    uint shadowLightIndex;

    float cascadeSplit0;
    float cascadeSplit1;
    float cascadeSplit2;
    uint cascadeDebugEnabled;
    
    uint cascadeUpdateMask;
    uint cascadeUpdatePadding0;
    uint cascadeUpdatePadding1;
    uint cascadeUpdatePadding2;
};

float LinearizeDepth(float depth)
{
    float denominator = depth + cameraProjection._33;

    if (abs(denominator) < 0.000001f)
    {
        denominator = denominator < 0.0f ? -0.000001f : 0.000001f;
    }

    return -cameraProjection._43 / denominator;
}

void GetVisibleShadowDepthRange(out float nearDistance, out float farDistance)
{
    float2 minMaxDepth = inputMinMax.Load(int3(0, 0, 0));

    nearDistance = max(-LinearizeDepth(minMaxDepth.x), 0.0001f);
    farDistance = max(-LinearizeDepth(minMaxDepth.y), nearDistance + 0.0001f);
}

// Camera-space depth limit for shadow receivers.
// Initial quality setting for this level, in world units.
static const float SHADOW_RECEIVER_MAX_DEPTH = 50.0f;

void GetShadowReceiverDepthRange(out float nearDistance, out float farDistance)
{
    float visibleNear;
    float visibleFar;

    GetVisibleShadowDepthRange(visibleNear, visibleFar);

    float cameraNear = max(-LinearizeDepth(0.0f), 0.0001f);

    // Keep a valid interval even when all visible geometry
    // lies beyond the configured shadow distance.
    float receiverLimit = max(SHADOW_RECEIVER_MAX_DEPTH, cameraNear + 0.01f);

    nearDistance = clamp(visibleNear, cameraNear, receiverLimit - 0.01f);

    farDistance = clamp(visibleFar, nearDistance + 0.01f, receiverLimit);
}

void BuildFrustumCorners(float nearDistance, float farDistance, out float3 corners[8])
{
    const float xScale = cameraProjection._11;
    const float yScale = cameraProjection._22;

    uint cornerIndex = 0;

    [unroll]
    for (uint depthIndex = 0; depthIndex < 2; ++depthIndex)
    {
        const float distanceValue = depthIndex == 0 ? nearDistance : farDistance;

        [unroll]
        for (uint yIndex = 0; yIndex < 2; ++yIndex)
        {
            const float ndcY = yIndex == 0 ? -1.0f : 1.0f;

            [unroll]
            for (uint xIndex = 0; xIndex < 2; ++xIndex)
            {
                const float ndcX = xIndex == 0 ? -1.0f : 1.0f;

                float3 viewPoint;
                viewPoint.x = ndcX * distanceValue / xScale;
                viewPoint.y = ndcY * distanceValue / yScale;
                viewPoint.z = -distanceValue;

                float4 worldPoint = mul(float4(viewPoint, 1.0f), inverseView);

                corners[cornerIndex] = worldPoint.xyz / worldPoint.w;
                ++cornerIndex;
            }
        }
    }
}

float4 ComputeBoundingSphere(float3 corners[8])
{
    float3 center = 0.0f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        center += corners[i];
    }

    center /= 8.0f;

    float radius = 0.0f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        radius = max(radius, distance(center, corners[i]));
    }

    radius = max(radius, 0.0001f);

    return float4(center, radius);
}

float4x4 BuildLookAtRH(float3 eye, float3 target, float3 up)
{
    float3 zAxis = normalize(eye - target);
    float3 xAxis = normalize(cross(up, zAxis));
    float3 yAxis = cross(zAxis, xAxis);

    return float4x4(
        xAxis.x, yAxis.x, zAxis.x, 0.0f,
        xAxis.y, yAxis.y, zAxis.y, 0.0f,
        xAxis.z, yAxis.z, zAxis.z, 0.0f,
        -dot(xAxis, eye), -dot(yAxis, eye), -dot(zAxis, eye), 1.0f
    );
}

float4x4 BuildOrthographicRH(float width, float height, float nearPlane, float farPlane)
{
    const float inverseDepthRange = 1.0f / (nearPlane - farPlane);

    return float4x4(
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, inverseDepthRange, 0.0f,
        0.0f, 0.0f, nearPlane * inverseDepthRange, 1.0f
    );
}

float4x4 BuildIdentityMatrix()
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

float4x4 BuildLightViewProjection(float nearDistance, float farDistance, uint cascadeIndex)
{
    float3 cascadeCorners[8];
    BuildFrustumCorners(nearDistance, farDistance, cascadeCorners);

    float4 cascadeSphere = ComputeBoundingSphere(cascadeCorners);

    float fullNearDistance;
    float fullFarDistance;
    GetVisibleShadowDepthRange(fullNearDistance, fullFarDistance);

    float3 fullCorners[8];
    BuildFrustumCorners(fullNearDistance, fullFarDistance, fullCorners);

    float4 fullSphere = ComputeBoundingSphere(fullCorners);

    float3 direction = normalize(lightDirection);
    float3 up = float3(0.0f, 1.0f, 0.0f);

    if (abs(direction.y) > 0.95f)
    {
        up = float3(0.0f, 0.0f, 1.0f);
    }

    float centerOffsetAlongLight = dot(cascadeSphere.xyz - fullSphere.xyz, direction);

    float eyeDistance = centerOffsetAlongLight + fullSphere.w + sunDistance;

    float3 eye = cascadeSphere.xyz - direction * eyeDistance;

    float4x4 lightView = BuildLookAtRH(eye, eye + direction, up);

    uint baseResolution = (uint) round(1.0f / shadowMapTexelSize);
    uint resolution = max(1u, baseResolution / (uint) SHADOW_CASCADE_DIVISOR(cascadeIndex));

    float orthoSize = cascadeSphere.w * 2.0f;

    if (resolution > 1u)
    {
        orthoSize *= float(resolution) / float(resolution - 1u);
    }

    float depthRange = fullSphere.w * 2.0f + sunDistance;

    float4x4 lightProjection = BuildOrthographicRH(orthoSize, orthoSize, 0.0f, depthRange);

    float4x4 lightViewProjection =
        mul(lightView, lightProjection);

    if (resolution > 1u)
    {
        float4 projectedOrigin = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), lightViewProjection);

        float2 originInTexels = (projectedOrigin.xy / projectedOrigin.w) * (0.5f * float(resolution));

        float2 snappedOrigin = round(originInTexels);

        float2 offsetNDC = (snappedOrigin - originInTexels) * (2.0f / float(resolution));

        lightViewProjection._41 += offsetNDC.x;
        lightViewProjection._42 += offsetNDC.y;
    }

    return lightViewProjection;
}

ShadowDataOutput BuildShadowOutput(float4x4 lightViewProjection, uint enabled)
{
    ShadowDataOutput output;

    output.lightViewProjection = lightViewProjection;

    output.shadowBias = shadowBias;
    output.shadowStrength = shadowStrength;
    output.shadowsEnabled = enabled;
    output.paddingShadow = 0.0f;

    output.shadowMapTexelSize = float2(shadowMapTexelSize, shadowMapTexelSize);
    output.pcfEnabled = pcfEnabled;
    output.pcfRadius = pcfRadius;

    output.cascadeCount = clamp(cascadeCount, 1u, (uint) MAX_SHADOW_CASCADES);
    output.cascadeFitMode = cascadeFitMode;

    // x = debug cascade tint enabled
    // y = unused
    output.cascadePadding = float2(cascadeDebugEnabled != 0u ? 1.0f : 0.0f, 0.0f);

    output.cascadeFarDistances = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4x4 identityMatrix = BuildIdentityMatrix();

    output.cascadeLightViewProjection[0] = identityMatrix;
    output.cascadeLightViewProjection[1] = identityMatrix;
    output.cascadeLightViewProjection[2] = identityMatrix;
    output.cascadeLightViewProjection[3] = identityMatrix;
    
    output.shadowCameraView = cameraView;

    output.cascadeWorldTexelSize = float4(shadowMapTexelSize, shadowMapTexelSize, shadowMapTexelSize, shadowMapTexelSize);

    output.cascadeDepthRanges = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.shadowLightDirection = float4(normalize(lightDirection), 0.0f);

    output.shadowLightIndex = shadowLightIndex;
    
    output.cascadeBlendFraction = 0.0f;
    output.normalBiasTexels = 0.0f;
    output.slopeBiasTexels = 0.0f;

    return output;
}

[numthreads(1, 1, 1)]
void main()
{
    float2 minMaxDepth = inputMinMax.Load(int3(0, 0, 0));

    if (minMaxDepth.x > minMaxDepth.y)
    {
        outputShadowData[0] = BuildShadowOutput(BuildIdentityMatrix(), 0u);
        return;
    }

    ShadowDataOutput previousOutput = outputShadowData[0];

    float nearDistance;
    float farDistance;

    GetShadowReceiverDepthRange(nearDistance, farDistance);

    float4x4 fullLightViewProjection = BuildLightViewProjection(nearDistance, farDistance, 0u);

    ShadowDataOutput output = BuildShadowOutput(fullLightViewProjection, 1u);
    
    // cascadePadding.x = debug enabled
    // cascadePadding.y = camera near distance used to fit the cascades
    output.cascadePadding.y = nearDistance;

    uint activeCascadeCount = output.cascadeCount;
    float fittedDepthRange = farDistance - nearDistance;

    // Cascade 0
    float cascade0FarFraction = activeCascadeCount > 1 ? cascadeSplit0 : 1.0f;
    float cascade0NearDistance = nearDistance;
    float cascade0FarDistance = nearDistance + fittedDepthRange * cascade0FarFraction;

    cascade0FarDistance = max(cascade0FarDistance, cascade0NearDistance + 0.0001f);

    output.cascadeFarDistances.x = cascade0FarDistance;
    output.cascadeLightViewProjection[0] = BuildLightViewProjection(cascade0NearDistance, cascade0FarDistance, 0u);

    // Cascade 1
    if (activeCascadeCount > 1)
    {
        float cascade1FarFraction = activeCascadeCount > 2 ? cascadeSplit1 : 1.0f;
        float cascade1NearDistance = output.cascadeFitMode == CASCADE_FIT_TO_CASCADE ? cascade0FarDistance : nearDistance;
        float cascade1FarDistance = nearDistance + fittedDepthRange * cascade1FarFraction;

        cascade1FarDistance = max(cascade1FarDistance, cascade1NearDistance + 0.0001f);

        output.cascadeFarDistances.y = cascade1FarDistance;
        output.cascadeLightViewProjection[1] = BuildLightViewProjection(cascade1NearDistance, cascade1FarDistance, 1u);

        // Cascade 2
        if (activeCascadeCount > 2)
        {
            float cascade2FarFraction = activeCascadeCount > 3 ? cascadeSplit2 : 1.0f;
            float cascade2NearDistance = output.cascadeFitMode == CASCADE_FIT_TO_CASCADE ? cascade1FarDistance : nearDistance;
            float cascade2FarDistance = nearDistance + fittedDepthRange * cascade2FarFraction;

            cascade2FarDistance = max(cascade2FarDistance, cascade2NearDistance + 0.0001f);

            output.cascadeFarDistances.z = cascade2FarDistance;
            output.cascadeLightViewProjection[2] = BuildLightViewProjection(cascade2NearDistance, cascade2FarDistance, 2u);

            // Cascade 3
            if (activeCascadeCount > 3)
            {
                float cascade3NearDistance = output.cascadeFitMode == CASCADE_FIT_TO_CASCADE ? cascade2FarDistance : nearDistance;
                float cascade3FarDistance = farDistance;

                cascade3FarDistance = max(cascade3FarDistance, cascade3NearDistance + 0.0001f);

                output.cascadeFarDistances.w = cascade3FarDistance;
                output.cascadeLightViewProjection[3] = BuildLightViewProjection(cascade3NearDistance, cascade3FarDistance, 3u);
            }
        }
    }

    if ((cascadeUpdateMask & (1u << 0)) == 0u)
    {
        output.cascadeFarDistances.x = previousOutput.cascadeFarDistances.x;
        output.cascadeLightViewProjection[0] = previousOutput.cascadeLightViewProjection[0];
    }

    if ((cascadeUpdateMask & (1u << 1)) == 0u)
    {
        output.cascadeFarDistances.y = previousOutput.cascadeFarDistances.y;
        output.cascadeLightViewProjection[1] = previousOutput.cascadeLightViewProjection[1];
    }

    if ((cascadeUpdateMask & (1u << 2)) == 0u)
    {
        output.cascadeFarDistances.z = previousOutput.cascadeFarDistances.z;
        output.cascadeLightViewProjection[2] = previousOutput.cascadeLightViewProjection[2];
    }

    if ((cascadeUpdateMask & (1u << 3)) == 0u)
    {
        output.cascadeFarDistances.w = previousOutput.cascadeFarDistances.w;
        output.cascadeLightViewProjection[3] = previousOutput.cascadeLightViewProjection[3];
    }

    // Diagnostic metadata derived from the final orthographic matrices.
    output.cascadeWorldTexelSize = float4(0, 0, 0, 0);
    output.cascadeDepthRanges = float4(0, 0, 0, 0);

    uint baseResolution =
    (uint) round(1.0f / shadowMapTexelSize);

    for (uint i = 0u; i < output.cascadeCount; ++i)
    {
        float4x4 vp = output.cascadeLightViewProjection[i];

    // Row-vector convention: these columns describe the
    // world-space gradients of projected X and Z.
        float scaleX = length(float3(vp._11, vp._21, vp._31));
        float scaleZ = length(float3(vp._13, vp._23, vp._33));

        float widthWorld = 2.0f / max(scaleX, 0.00000001f);
        float depthRangeWorld = 1.0f / max(scaleZ, 0.00000001f);

        uint resolution = max(
        1u,
        baseResolution / (uint) SHADOW_CASCADE_DIVISOR(i));

        output.cascadeWorldTexelSize[i] =
        widthWorld / float(resolution);

        output.cascadeDepthRanges[i] = depthRangeWorld;
    }

    outputShadowData[0] = output;
    
}