#include "VolumetricFogCommon.hlsli"

cbuffer IntegrationConstants : register(b0)
{
    float2 projectionScale;
    float nearDistance;
    float maxDistance;

    uint gridWidth;
    uint gridHeight;
    uint gridDepth;
    uint padding;
};

Texture3D<float4> mediumVolume : register(t0);
Texture3D<float4> lightingVolume : register(t1);
RWTexture3D<float4> integratedVolume : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight)
        return;

    uint2 gridSize = uint2(gridWidth, gridHeight);
    float2 ndc = GetFroxelNDC(dispatchThreadID.xy, gridSize);
    float3 viewRay = float3(ndc.x / projectionScale.x, ndc.y / projectionScale.y, -1.0f);
    float rayLengthScale = length(viewRay);

    float3 accumulatedScattering = 0.0f;
    float accumulatedTransmittance = 1.0f;

    [loop]
    for (uint z = 0; z < gridDepth; ++z)
    {
        uint3 coordinate = uint3(dispatchThreadID.xy, z);

        float4 medium = mediumVolume.Load(int4(coordinate, 0));
        float3 inScattering = lightingVolume.Load(int4(coordinate, 0)).rgb;

        float startDepth = GetFroxelBoundaryDepth(z, gridDepth, nearDistance, maxDistance);
        float endDepth = GetFroxelBoundaryDepth(z + 1, gridDepth, nearDistance, maxDistance);
        float segmentLength = max((endDepth - startDepth) * rayLengthScale, 0.0f);

        float extinction = max(medium.a, 0.0f);
        float segmentTransmittance = exp(-extinction * segmentLength);

        float3 segmentScattering = extinction > 0.000001f ? inScattering * ((1.0f - segmentTransmittance) / extinction) : inScattering * segmentLength;

        accumulatedScattering += accumulatedTransmittance * segmentScattering;
        accumulatedTransmittance *= segmentTransmittance;

        integratedVolume[coordinate] = float4(accumulatedScattering, accumulatedTransmittance);
    }
}