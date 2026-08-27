#include "VolumetricFogCommon.hlsli"

cbuffer MediumConstants : register(b0)
{
    float4x4 inverseView;

    float2 projectionScale;
    float nearDistance;
    float maxDistance;

    float density;
    float scatteringCoefficient;
    float extinctionCoefficient;
    float noiseScale;

    float noiseStrength;
    float animationTime;
    float windSpeed;
    uint animateDensity;

    float3 windDirection;
    uint gridWidth;

    uint gridHeight;
    uint gridDepth;
    uint padding0;
    uint padding1;
};

RWTexture3D<float4> mediumVolume : register(u0);

uint HashLattice(int3 p)
{
    uint3 q = asuint(p);

    uint h = q.x * 374761393u;
    h += q.y * 668265263u;
    h += q.z * 2246822519u;
    h = (h ^ (h >> 13u)) * 1274126177u;

    return h ^ (h >> 16u);
}

float PerlinGradient(uint hash, float3 p)
{
    uint h = hash & 15u;

    float u = h < 8u ? p.x : p.y;
    float v = h < 4u ? p.y : ((h == 12u || h == 14u) ? p.x : p.z);

    float a = (h & 1u) != 0u ? -u : u;
    float b = (h & 2u) != 0u ? -v : v;

    return a + b;
}

float3 PerlinFade(float3 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise(float3 position)
{
    int3 cell = (int3) floor(position);
    float3 local = frac(position);
    float3 fade = PerlinFade(local);

    float n000 = PerlinGradient(HashLattice(cell + int3(0, 0, 0)), local - float3(0.0f, 0.0f, 0.0f));
    float n100 = PerlinGradient(HashLattice(cell + int3(1, 0, 0)), local - float3(1.0f, 0.0f, 0.0f));
    float n010 = PerlinGradient(HashLattice(cell + int3(0, 1, 0)), local - float3(0.0f, 1.0f, 0.0f));
    float n110 = PerlinGradient(HashLattice(cell + int3(1, 1, 0)), local - float3(1.0f, 1.0f, 0.0f));

    float n001 = PerlinGradient(HashLattice(cell + int3(0, 0, 1)), local - float3(0.0f, 0.0f, 1.0f));
    float n101 = PerlinGradient(HashLattice(cell + int3(1, 0, 1)), local - float3(1.0f, 0.0f, 1.0f));
    float n011 = PerlinGradient(HashLattice(cell + int3(0, 1, 1)), local - float3(0.0f, 1.0f, 1.0f));
    float n111 = PerlinGradient(HashLattice(cell + int3(1, 1, 1)), local - float3(1.0f, 1.0f, 1.0f));

    float nx00 = lerp(n000, n100, fade.x);
    float nx10 = lerp(n010, n110, fade.x);
    float nx01 = lerp(n001, n101, fade.x);
    float nx11 = lerp(n011, n111, fade.x);

    float nxy0 = lerp(nx00, nx10, fade.y);
    float nxy1 = lerp(nx01, nx11, fade.y);

    return saturate(0.5f + 0.5f * lerp(nxy0, nxy1, fade.z));
}

[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight || dispatchThreadID.z >= gridDepth)
        return;

    float effectiveDensity = density;

    if (animateDensity != 0)
    {
        uint3 gridSize = uint3(gridWidth, gridHeight, gridDepth);
        float3 worldPosition = GetFroxelWorldPosition(dispatchThreadID, gridSize, projectionScale, nearDistance, maxDistance, inverseView);

        float3 wind = windDirection;
        float windLength = length(wind);

        if (windLength > 0.000001f)
            wind /= windLength;
        else
            wind = float3(0.0f, 0.0f, 0.0f);

        float3 noisePosition = (worldPosition - wind * animationTime * windSpeed) * noiseScale;
        float noise = PerlinNoise(noisePosition);
        float densityMultiplier = lerp(1.0f, noise * 2.0f, saturate(noiseStrength));

        effectiveDensity *= max(densityMultiplier, 0.0f);
    }

    float effectiveScattering = effectiveDensity * scatteringCoefficient;
    float effectiveExtinction = effectiveDensity * extinctionCoefficient;

    mediumVolume[dispatchThreadID] = float4(effectiveScattering.xxx, effectiveExtinction);
}