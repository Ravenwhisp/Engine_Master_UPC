#ifndef VOLUMETRIC_FOG_LIGHTING_DATA_HLSLI
#define VOLUMETRIC_FOG_LIGHTING_DATA_HLSLI
#include "VolumetricFogLightBlocks.h"

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
    uint pointLightCullingEnabled;
    uint optimizedShadowFilterEnabled;
    uint shadowDiagnostic;
    uint shadowSampleCount;
    uint2 shadowDiagnosticPadding;
    float heightFogDensity;
    float heightFogBaseHeight;
    float heightFogFalloff;
    float uniformFogDensity;
};

#define MAX_POINT_LIGHTS FOG_POINT_LIGHT_CAPACITY
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

uint GetFogLightBlockIndex(uint3 block)
{
    uint countX = (gridWidth + FOG_LIGHT_BLOCK_X - 1) / FOG_LIGHT_BLOCK_X;
    uint countY = (gridHeight + FOG_LIGHT_BLOCK_Y - 1) / FOG_LIGHT_BLOCK_Y;
    return block.x + countX * (block.y + countY * block.z);
}
#endif
