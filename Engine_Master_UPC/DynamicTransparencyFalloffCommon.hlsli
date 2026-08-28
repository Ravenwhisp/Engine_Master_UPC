#ifndef DYNAMIC_TRANSPARENCY_FALLOFF_COMMON_HLSLI
#define DYNAMIC_TRANSPARENCY_FALLOFF_COMMON_HLSLI

struct DynamicTransparencyMaterialData
{
    float3 diffuseColour;
    uint hasDiffuseTex;

    float metallicFactor;
    float roughnessFactor;
    uint hasMetallicRoughnessTex;
    float normalFactor;

    uint hasNormalTex;
    float3 emmisiveColour;

    uint hasEmissiveTex;
    float3 padding;
};

cbuffer DynamicTransparencyModelCB : register(b4)
{
    float4x4 model;
    float4x4 normalMat;
    DynamicTransparencyMaterialData material;
};

#endif