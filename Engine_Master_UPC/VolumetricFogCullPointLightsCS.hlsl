#include "VolumetricFogCommon.hlsli"
#include "VolumetricFogLightingData.hlsli"

RWStructuredBuffer<uint> blockLightMasks : register(u1);
groupshared float3 blockMin;
groupshared float3 blockMax;
groupshared uint lightMasks[FOG_POINT_LIGHT_MASK_WORDS];

[numthreads(64, 1, 1)]
void main(uint3 block : SV_GroupID, uint lane : SV_GroupIndex)
{
    if (lane < FOG_POINT_LIGHT_MASK_WORDS)
        lightMasks[lane] = 0u;

    if (lane == 0u)
    {
        uint3 first = block * uint3(FOG_LIGHT_BLOCK_X, FOG_LIGHT_BLOCK_Y, FOG_LIGHT_BLOCK_Z);
        uint3 last = min(first + uint3(FOG_LIGHT_BLOCK_X, FOG_LIGHT_BLOCK_Y, FOG_LIGHT_BLOCK_Z), uint3(gridWidth, gridHeight, gridDepth));
        float2 uv0 = float2(first.xy) / float2(gridWidth, gridHeight);
        float2 uv1 = float2(last.xy) / float2(gridWidth, gridHeight);
        float z0 = GetFroxelBoundaryDepth(first.z, gridDepth, nearDistance, maxDistance);
        float z1 = GetFroxelBoundaryDepth(last.z, gridDepth, nearDistance, maxDistance);
        float3 boundsMin = float3(3.402823466e+38f, 3.402823466e+38f, 3.402823466e+38f);
        float3 boundsMax = -boundsMin;
        // Enclose the whole frustum block, including every jitter position.
        [unroll]
        for (uint corner = 0u; corner < 8u; ++corner)
        {
            float2 uv = float2((corner & 1u) ? uv1.x : uv0.x, (corner & 2u) ? uv1.y : uv0.y);
            float depth = (corner & 4u) ? z1 : z0;
            float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
            float3 viewPosition = float3(ndc * depth / projectionScale, -depth);
            float4 world = mul(float4(viewPosition, 1.0f), inverseView);
            float3 position = world.xyz / world.w;
            boundsMin = min(boundsMin, position);
            boundsMax = max(boundsMax, position);
        }
        // Expand for floating-point differences between corner and froxel transforms.
        float3 margin = 1.0e-4f + max(abs(boundsMin), abs(boundsMax)) * 1.0e-5f;
        blockMin = boundsMin - margin;
        blockMax = boundsMax + margin;
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint index = lane; index < min(pointLightCount, (uint) MAX_POINT_LIGHTS); index += 64u)
    {
        PointLight light = pointLights[index];
        float3 delta = light.position - clamp(light.position, blockMin, blockMax);
        if (dot(delta, delta) <= light.radius * light.radius)
            InterlockedOr(lightMasks[index / 32u], 1u << (index % 32u));
    }
    GroupMemoryBarrierWithGroupSync();
    if (lane < FOG_POINT_LIGHT_MASK_WORDS)
        blockLightMasks[GetFogLightBlockIndex(block) * FOG_POINT_LIGHT_MASK_WORDS + lane] = lightMasks[lane];
}
