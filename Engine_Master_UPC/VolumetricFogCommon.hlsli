#ifndef VOLUMETRIC_FOG_COMMON_HLSLI
#define VOLUMETRIC_FOG_COMMON_HLSLI

static const float VOLUMETRIC_FOG_MIN_DEPTH_RANGE = 0.001f;

float GetVolumetricDepth(float normalizedZ, float nearDistance, float maxDistance)
{
    float safeNear = max(nearDistance, VOLUMETRIC_FOG_MIN_DEPTH_RANGE);
    float safeFar = max(maxDistance, safeNear + VOLUMETRIC_FOG_MIN_DEPTH_RANGE);
    return safeNear * pow(safeFar / safeNear, saturate(normalizedZ));
}

float GetFroxelCenterDepth(uint z, uint gridDepth, float nearDistance, float maxDistance)
{
    float normalizedZ = (float(z) + 0.5f) / float(gridDepth);
    return GetVolumetricDepth(normalizedZ, nearDistance, maxDistance);
}

float GetFroxelBoundaryDepth(uint boundary, uint gridDepth, float nearDistance, float maxDistance)
{
    float normalizedZ = float(boundary) / float(gridDepth);
    return GetVolumetricDepth(normalizedZ, nearDistance, maxDistance);
}

float2 GetFroxelUV(uint2 froxelXY, uint2 gridSizeXY)
{
    return (float2(froxelXY) + 0.5f) / float2(gridSizeXY);
}

float2 GetFroxelNDC(uint2 froxelXY, uint2 gridSizeXY)
{
    float2 uv = GetFroxelUV(froxelXY, gridSizeXY);
    return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

float3 GetFroxelViewPosition(uint3 froxel, uint3 gridSize, float2 projectionScale, float nearDistance, float maxDistance)
{
    float2 ndc = GetFroxelNDC(froxel.xy, gridSize.xy);
    float depth = GetFroxelCenterDepth(froxel.z, gridSize.z, nearDistance, maxDistance);
    return float3(ndc.x * depth / projectionScale.x, ndc.y * depth / projectionScale.y, -depth);
}

float3 GetFroxelWorldPosition(uint3 froxel, uint3 gridSize, float2 projectionScale, float nearDistance, float maxDistance, float4x4 inverseView)
{
    float3 viewPosition = GetFroxelViewPosition(froxel, gridSize, projectionScale, nearDistance, maxDistance);
    float4 worldPosition = mul(float4(viewPosition, 1.0f), inverseView);
    return worldPosition.xyz / worldPosition.w;
}

#endif
