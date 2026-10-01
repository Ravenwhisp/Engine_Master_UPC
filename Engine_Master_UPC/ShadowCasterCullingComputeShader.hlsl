#define SHADOW_CB_REGISTER b0
#include "ShadowData.hlsli"

#define SHADOW_CASTER_FLAG_FORCE_VISIBLE 1u

struct ShadowCasterCandidateGPU
{
    float4 bounds[8];
    float4x4 model;

    uint vertexBufferAddressLow;
    uint vertexBufferAddressHigh;
    uint vertexBufferSize;
    uint vertexBufferStride;

    uint indexBufferAddressLow;
    uint indexBufferAddressHigh;
    uint indexBufferSize;
    uint indexBufferFormat;

    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;

    uint flags;
    uint padding0;
    uint padding1;
};

struct ShadowIndirectCommandGPU
{
    float4x4 model;

    uint vertexBufferAddressLow;
    uint vertexBufferAddressHigh;
    uint vertexBufferSize;
    uint vertexBufferStride;

    uint indexBufferAddressLow;
    uint indexBufferAddressHigh;
    uint indexBufferSize;
    uint indexBufferFormat;

    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

StructuredBuffer<ShadowCasterCandidateGPU> candidates : register(t0);

RWStructuredBuffer<uint> visibilityMasks : register(u0);
RWStructuredBuffer<uint> cascadeVisibleCounts : register(u1);
RWStructuredBuffer<ShadowIndirectCommandGPU> indirectCommands0 : register(u2);
RWStructuredBuffer<ShadowIndirectCommandGPU> indirectCommands1 : register(u3);
RWStructuredBuffer<ShadowIndirectCommandGPU> indirectCommands2 : register(u4);
RWStructuredBuffer<ShadowIndirectCommandGPU> indirectCommands3 : register(u5);



cbuffer ShadowCasterCullingParams : register(b1)
{
    uint candidateCount;
};

bool IntersectsCascade(ShadowCasterCandidateGPU candidate, uint cascadeIndex)
{
    bool outsideLeft = true;
    bool outsideRight = true;
    bool outsideBottom = true;
    bool outsideTop = true;
    bool outsideNear = true;
    bool outsideFar = true;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float4 clip = mul(candidate.bounds[i], cascadeLightViewProjection[cascadeIndex]);

        outsideLeft = outsideLeft && (clip.x < -clip.w);
        outsideRight = outsideRight && (clip.x > clip.w);
        outsideBottom = outsideBottom && (clip.y < -clip.w);
        outsideTop = outsideTop && (clip.y > clip.w);
        outsideNear = outsideNear && (clip.z < 0.0f);
        outsideFar = outsideFar && (clip.z > clip.w);
    }

    return !(outsideLeft || outsideRight || outsideBottom || outsideTop || outsideNear || outsideFar);
}

ShadowIndirectCommandGPU BuildIndirectCommand(ShadowCasterCandidateGPU candidate)
{
    ShadowIndirectCommandGPU command;

    command.model = candidate.model;

    command.vertexBufferAddressLow = candidate.vertexBufferAddressLow;
    command.vertexBufferAddressHigh = candidate.vertexBufferAddressHigh;
    command.vertexBufferSize = candidate.vertexBufferSize;
    command.vertexBufferStride = candidate.vertexBufferStride;

    command.indexBufferAddressLow = candidate.indexBufferAddressLow;
    command.indexBufferAddressHigh = candidate.indexBufferAddressHigh;
    command.indexBufferSize = candidate.indexBufferSize;
    command.indexBufferFormat = candidate.indexBufferFormat;

    command.indexCountPerInstance = candidate.indexCountPerInstance;
    command.instanceCount = candidate.instanceCount;
    command.startIndexLocation = candidate.startIndexLocation;
    command.baseVertexLocation = candidate.baseVertexLocation;
    command.startInstanceLocation = candidate.startInstanceLocation;

    return command;
}


[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint candidateIndex = dispatchThreadId.x;

    if (candidateIndex >= candidateCount)
    {
        return;
    }

    ShadowCasterCandidateGPU candidate = candidates[candidateIndex];

    const uint activeCascadeCount = min(cascadeCount, (uint) MAX_SHADOW_CASCADES);

    uint visibilityMask = 0;

    if ((candidate.flags & SHADOW_CASTER_FLAG_FORCE_VISIBLE) != 0)
    {
        visibilityMask = (1u << activeCascadeCount) - 1u;
    }
    else
    {
        [unroll]
        for (uint cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
        {
            if (cascadeIndex >= activeCascadeCount)
            {
                break;
            }

            if (IntersectsCascade(candidate, cascadeIndex))
            {
                visibilityMask |= 1u << cascadeIndex;
            }
        }
    }

    visibilityMasks[candidateIndex] = visibilityMask;

    ShadowIndirectCommandGPU indirectCommand = BuildIndirectCommand(candidate);

[unroll]
    for (uint cascadeIndex = 0; cascadeIndex < MAX_SHADOW_CASCADES; ++cascadeIndex)
    {
        if ((visibilityMask & (1u << cascadeIndex)) == 0)
        {
            continue;
        }

        uint commandIndex = 0;
        InterlockedAdd(cascadeVisibleCounts[cascadeIndex], 1, commandIndex);

        switch (cascadeIndex)
        {
            case 0:
                indirectCommands0[commandIndex] = indirectCommand;
                break;

            case 1:
                indirectCommands1[commandIndex] = indirectCommand;
                break;

            case 2:
                indirectCommands2[commandIndex] = indirectCommand;
                break;

            case 3:
                indirectCommands3[commandIndex] = indirectCommand;
                break;
        }
    }
}