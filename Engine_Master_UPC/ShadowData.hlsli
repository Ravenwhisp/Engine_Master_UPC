#ifndef SHADOW_DATA_HLSLI
#define SHADOW_DATA_HLSLI
#ifndef MAX_SHADOW_CASCADES
#define MAX_SHADOW_CASCADES 4
#endif
#ifndef SHADOW_CB_REGISTER
#define SHADOW_CB_REGISTER b3
#endif
cbuffer ShadowData : register(SHADOW_CB_REGISTER)
{
    float4x4 lightViewProjection;
    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;
    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;
    uint cascadeCount;
    uint cascadeFitMode;
    float2 cascadePadding;
    float4 cascadeFarDistances;
    float4x4 cascadeLightViewProjection[MAX_SHADOW_CASCADES];
    float4x4 shadowCameraView;
    float4 cascadeWorldTexelSize;
    float4 cascadeDepthRanges;
    float4 shadowLightDirection;
    uint shadowLightIndex;
    float cascadeBlendFraction;
    float normalBiasTexels;
    float slopeBiasTexels;
};
#endif
