#ifndef VOLUMETRIC_FOG_SAMPLING_HLSLI
#define VOLUMETRIC_FOG_SAMPLING_HLSLI
#include "VolumetricFogCommon.hlsli"

// Packed 128x128 toroidal void-and-cluster rank tile (four UNORM8 values/word).
ByteAddressBuffer fogBlueNoise : register(t2);

float GetFogSampleOffset(uint3 froxel, uint mode, float strength)
{
    // Mode 0: centered; 1: original IGN; 2: blue noise.
    if (mode != 2)
        return GetFroxelSampleOffset(froxel.xy, mode, strength);

    // Stable in time; decorrelate depth layers without changing density animation.
    uint seed = froxel.z * 747796405u + 2891336453u;
    seed = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    seed = (seed >> 22u) ^ seed;
    uint2 pixel = (froxel.xy + uint2(seed, seed >> 16u)) & 127u;
    uint index = pixel.y * 128u + pixel.x;
    uint packed = fogBlueNoise.Load((index >> 2u) * 4u);
    float jitter = (float((packed >> ((index & 3u) * 8u)) & 255u) + 0.5f) / 256.0f;
    return lerp(0.5f, jitter, saturate(strength));
}
#endif
