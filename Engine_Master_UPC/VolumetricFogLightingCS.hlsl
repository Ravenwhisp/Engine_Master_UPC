#include "VolumetricFogCommon.hlsli"

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
    float3 inScattering = medium.rgb * lightColor * lightIntensity * phase;

    lightingVolume[dispatchThreadID] = float4(inScattering, 0.0f);
}