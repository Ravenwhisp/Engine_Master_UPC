cbuffer MVP : register(b0)
{
    float4x4 mvp;
};

cbuffer ModelData : register(b2)
{
    float4x4 model;
    float4x4 normalMat;
    float3 diffuseColor;
    bool hasDiffuseTex;
    float metallicFactor;
    float roughnessFactor;
    bool hasMetallicRoughnessTex;
    float normalFactor;
    uint hasNormalTex;
    float3 emissiveColor;
    uint hasEmissiveTex;
    float3 modelPadding;
};

cbuffer FlowMapData : register(b6)
{
    float2 flowDirection;
    float2 flowTiling;
    float2 flowOffset;
    float flowStrength;
    uint flowSource;
    uint flowEnabled;
    float2 flowPadding;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 coord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

VSOutput main(float3 position : POSITION, float2 coord : TEXCOORD,
              float3 normal : NORMAL, float3 tangent : TANGENT)
{
    VSOutput output;
    output.position = mul(float4(position, 1.0f), mvp);
    output.worldPos = mul(float4(position, 1.0f), model).xyz;
    output.coord = coord * flowTiling + flowOffset;
    output.normal = normalize(mul(normal, (float3x3)normalMat));
    output.tangent = normalize(mul(tangent, (float3x3)normalMat));
    return output;
}
