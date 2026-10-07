#include "VolumetricFogCommon.hlsli"
#define MAX_SHADOW_CASCADES 4

#define SHADOW_CB_REGISTER b1
#include "CascadeShadowMaps.hlsli"

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
    uint debugDisableShadows;

    float samplingJitterStrength;
    uint samplingJitterEnabled;
    uint samplingPadding0;
    uint samplingPadding1;
};

#define MAX_POINT_LIGHTS 256

struct PointLight
{
    float3 position;
    float radius;

    float3 color;
    float intensity;
};

cbuffer PointLightsCB : register(b2)
{
    uint pointLightCount;
    uint3 pointLightPadding;

    PointLight pointLights[MAX_POINT_LIGHTS];
};

#define MAX_SPOT_LIGHTS 64

struct SpotLight
{
    float3 position;
    float radius;

    float3 direction;
    float padding0;

    float3 color;
    float intensity;

    float cosineInnerAngle;
    float cosineOuterAngle;
    float2 padding1;
};

cbuffer SpotLightsCB : register(b3)
{
    uint spotLightCount;
    uint3 spotLightPadding;

    SpotLight spotLights[MAX_SPOT_LIGHTS];
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

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= 0.000001f)
        return 0.0f;

    float normalizedDistance = distanceValue / radiusValue;
    float normalizedDistance2 = normalizedDistance * normalizedDistance;
    float normalizedDistance4 = normalizedDistance2 * normalizedDistance2;

    float numerator = max(1.0f - normalizedDistance4, 0.0f);
    numerator *= numerator;

    float denominator = distanceValue * distanceValue + 1.0f;

    return numerator / denominator;
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, 0.000001f);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 ComputePointVolumetricScattering(PointLight light, float3 worldPosition, float3 viewDirection, float3 mediumScattering)
{
    if (light.radius <= 0.000001f)
        return 0.0f;

    float3 toFroxel = worldPosition - light.position;
    float distanceToFroxel = length(toFroxel);

    if (distanceToFroxel <= 0.000001f || distanceToFroxel >= light.radius)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceToFroxel, light.radius);

    if (attenuation <= 0.0f)
        return 0.0f;

    float3 incomingDirection = toFroxel / distanceToFroxel;
    float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

    return mediumScattering * light.color * light.intensity * attenuation * phase;
}

float3 ComputeSpotVolumetricScattering(SpotLight light, float3 worldPosition, float3 viewDirection, float3 mediumScattering)
{
    if (light.radius <= 0.000001f)
        return 0.0f;

    float3 spotDirection = normalize(light.direction);
    float3 toFroxel = worldPosition - light.position;

    float distanceProjected = dot(toFroxel, spotDirection);

    if (distanceProjected <= 0.0f || distanceProjected >= light.radius)
        return 0.0f;

    float distanceToFroxel = length(toFroxel);

    if (distanceToFroxel <= 0.000001f)
        return 0.0f;

    float3 incomingDirection = toFroxel / distanceToFroxel;

    float cosineAngle = dot(incomingDirection, spotDirection);

    if (cosineAngle <= light.cosineOuterAngle)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceProjected, light.radius);

    if (attenuation <= 0.0f)
        return 0.0f;

    float coneAttenuation = SpotConeAttenuation(cosineAngle, light.cosineInnerAngle, light.cosineOuterAngle);

    if (coneAttenuation <= 0.0f)
        return 0.0f;

    float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

    return mediumScattering * light.color * light.intensity * attenuation * coneAttenuation * phase;
}


[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight || dispatchThreadID.z >= gridDepth)
        return;

    uint3 gridSize = uint3(gridWidth, gridHeight, gridDepth);

    float sampleOffset = GetFroxelSampleOffset(dispatchThreadID.xy, samplingJitterEnabled, samplingJitterStrength);

    float4 medium = mediumVolume.Load(int4(dispatchThreadID, 0));

    float3 worldPosition = GetFroxelWorldPositionJittered(
    dispatchThreadID,
    gridSize,
    projectionScale,
    nearDistance,
    maxDistance,
    inverseView,
    sampleOffset);

    float3 viewDirection = normalize(cameraPosition - worldPosition);

    float3 inScattering = 0.0f;

    if (hasDirectionalLight != 0)
    {
        float3 incomingDirection = normalize(lightDirection);
        float phase = HenyeyGreenstein(dot(incomingDirection, viewDirection), anisotropy);

        float shadow = 1.0f;

        if (debugDisableShadows == 0)
        {
            uint selectedCascadeIndex;
            shadow = ComputeShadow(worldPosition, float3(0.0f, 0.0f, 0.0f), selectedCascadeIndex);
        }

        inScattering += medium.rgb * lightColor * lightIntensity * phase * shadow;
    }

    uint activePointLightCount = min(pointLightCount, (uint) MAX_POINT_LIGHTS);

    [loop]
    for (uint pointIndex = 0; pointIndex < activePointLightCount; ++pointIndex)
    {
        inScattering += ComputePointVolumetricScattering(pointLights[pointIndex], worldPosition, viewDirection, medium.rgb);
    }
    
    uint activeSpotLightCount = min(spotLightCount, (uint) MAX_SPOT_LIGHTS);

    [loop]
    for (uint spotIndex = 0; spotIndex < activeSpotLightCount; ++spotIndex)
    {
        inScattering += ComputeSpotVolumetricScattering(spotLights[spotIndex], worldPosition, viewDirection, medium.rgb);
    }

    lightingVolume[dispatchThreadID] = float4(inScattering, 0.0f);
}