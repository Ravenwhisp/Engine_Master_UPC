#pragma once

#include <cstdint>

static constexpr uint32_t SHADOW_CASTER_FLAG_FORCE_VISIBLE = 1u << 0;

struct ShadowCasterCandidateGPU
{
    // Eight world-space bounding-box corners.
    float bounds[8][4];

    // Pre-transposed model matrix, ready for the shadow VS root constants.
    float model[16];

    // D3D12_VERTEX_BUFFER_VIEW
    uint32_t vertexBufferAddressLow;
    uint32_t vertexBufferAddressHigh;
    uint32_t vertexBufferSize;
    uint32_t vertexBufferStride;

    // D3D12_INDEX_BUFFER_VIEW
    uint32_t indexBufferAddressLow;
    uint32_t indexBufferAddressHigh;
    uint32_t indexBufferSize;
    uint32_t indexBufferFormat;

    // D3D12_DRAW_INDEXED_ARGUMENTS
    uint32_t indexCountPerInstance;
    uint32_t instanceCount;
    uint32_t startIndexLocation;
    int32_t baseVertexLocation;
    uint32_t startInstanceLocation;

    uint32_t flags;
    uint32_t padding0;
    uint32_t padding1;
};

static_assert(sizeof(ShadowCasterCandidateGPU) == 256);