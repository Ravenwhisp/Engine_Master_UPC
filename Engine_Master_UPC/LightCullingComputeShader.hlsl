#include "LightingCBuffers.hlsli"
#include "LightTileCullingCommon.hlsli"

Texture2D<float> depthTexture : register(t0);

RWStructuredBuffer<int> pointLightIndices : register(u0);
RWStructuredBuffer<int> spotLightIndices : register(u1);

cbuffer TileCullingConstants : register(b0)
{
    float4x4 view;

    float xScale;
    float yScale;
    float proj33;
    float proj43;

    uint tileCountX;
    uint tileCountY;
    uint screenWidth;
    uint screenHeight;
};

groupshared uint gMinDepth;
groupshared uint gMaxDepth;
groupshared float3 gSidePlanes[4]; // left, right, top, bottom - all pass through the origin, so D=0
groupshared float gNearViewZ;
groupshared float gFarViewZ;
groupshared uint gPointCount;
groupshared uint gSpotCount;

// camera projection is right-handed (SimpleMath default) - forward is -Z, viewZ is negative in front of camera
float GetViewZ(float ndcDepth)
{
    return -proj43 / (ndcDepth + proj33);
}

float3 GetViewPosition(float2 ndc, float viewZ)
{
    float3 viewPos;
    viewPos.x = -ndc.x * viewZ / xScale;
    viewPos.y = -ndc.y * viewZ / yScale;
    viewPos.z = viewZ;
    return viewPos;
}

// tight bounding sphere of the spot cone, so culling doesn't over-include on wide-angle lights
float4 GetSpotBoundingSphere(float3 position, float3 direction, float range, float outerCosine)
{
    const float COS_45_DEG = 0.70710678f;

    if (outerCosine < COS_45_DEG)
    {
        const float sinAngle = sqrt(saturate(1.0f - outerCosine * outerCosine));
        const float radius = range * sinAngle / max(outerCosine, 1e-4f);
        return float4(position + direction * range, radius);
    }

    const float radius = range * 0.5f / (outerCosine * outerCosine);
    return float4(position + direction * radius, radius);
}

bool SphereIntersectsTile(float3 viewSpaceCenter, float radius)
{
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        if (dot(gSidePlanes[i], viewSpaceCenter) >= radius)
        {
            return false;
        }
    }

    // RH: both near/far viewZ are negative, near closer to 0 than far
    return (viewSpaceCenter.z - radius <= gNearViewZ) && (viewSpaceCenter.z + radius >= gFarViewZ);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint2 pixelCoord = dispatchThreadID.xy;
    const bool validPixel = pixelCoord.x < screenWidth && pixelCoord.y < screenHeight;

    if (groupIndex == 0)
    {
        gMinDepth = 0xffffffff;
        gMaxDepth = 0;
        gPointCount = 0;
        gSpotCount = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    const float depth = validPixel ? depthTexture.Load(int3(pixelCoord, 0)) : 1.0f;

    InterlockedMin(gMinDepth, asuint(depth));
    InterlockedMax(gMaxDepth, asuint(depth));

    GroupMemoryBarrierWithGroupSync();

    if (groupIndex == 0)
    {
        const float minDepth = asfloat(gMinDepth);
        const float maxDepth = asfloat(gMaxDepth);

        const uint2 tileMin = groupID.xy * TILE_SIZE;
        const uint2 tileMax = min(tileMin + TILE_SIZE, uint2(screenWidth, screenHeight));

        const float2 screenPoints[4] =
        {
            float2(tileMin.x, tileMin.y),
            float2(tileMax.x, tileMin.y),
            float2(tileMax.x, tileMax.y),
            float2(tileMin.x, tileMax.y)
        };

        const float nearViewZ = GetViewZ(minDepth);

        float3 viewPoints[4];

        [unroll]
        for (uint i = 0; i < 4; ++i)
        {
            float2 ndc;
            ndc.x = (screenPoints[i].x / (float) screenWidth) * 2.0f - 1.0f;
            ndc.y = 1.0f - (screenPoints[i].y / (float) screenHeight) * 2.0f;
            viewPoints[i] = GetViewPosition(ndc, nearViewZ);
        }

        // swapped cross order vs. the LH derivation - RH viewZ sign flip inverts winding
        [unroll]
        for (uint p = 0; p < 4; ++p)
        {
            gSidePlanes[p] = normalize(cross(viewPoints[(p + 1) & 3], viewPoints[p]));
        }

        gNearViewZ = nearViewZ;
        gFarViewZ = GetViewZ(maxDepth);
    }

    GroupMemoryBarrierWithGroupSync();

    const uint tileIndex = groupID.y * tileCountX + groupID.x;
    const uint tileBase = tileIndex * MAX_LIGHTS_PER_TILE;
    const uint threadsPerTile = TILE_SIZE * TILE_SIZE;

    for (uint pointIndex = groupIndex; pointIndex < pointCount; pointIndex += threadsPerTile)
    {
        const float3 viewSpacePos = mul(float4(pointLights[pointIndex].position, 1.0f), view).xyz;

        if (SphereIntersectsTile(viewSpacePos, pointLights[pointIndex].radius))
        {
            uint slot;
            InterlockedAdd(gPointCount, 1, slot);

            if (slot < MAX_LIGHTS_PER_TILE)
            {
                pointLightIndices[tileBase + slot] = (int) pointIndex;
            }
        }
    }

    for (uint spotIndex = groupIndex; spotIndex < spotCount; spotIndex += threadsPerTile)
    {
        const float4 bound = GetSpotBoundingSphere(spotLights[spotIndex].position, spotLights[spotIndex].direction,
            spotLights[spotIndex].radius, spotLights[spotIndex].cosineOuterAngle);
        const float3 viewSpacePos = mul(float4(bound.xyz, 1.0f), view).xyz;

        if (SphereIntersectsTile(viewSpacePos, bound.w))
        {
            uint slot;
            InterlockedAdd(gSpotCount, 1, slot);

            if (slot < MAX_LIGHTS_PER_TILE)
            {
                spotLightIndices[tileBase + slot] = (int) spotIndex;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (groupIndex == 0)
    {
        if (gPointCount < MAX_LIGHTS_PER_TILE)
        {
            pointLightIndices[tileBase + gPointCount] = -1;
        }

        if (gSpotCount < MAX_LIGHTS_PER_TILE)
        {
            spotLightIndices[tileBase + gSpotCount] = -1;
        }
    }
}
