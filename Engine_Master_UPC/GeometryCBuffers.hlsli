cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};

cbuffer SceneData : register(b1)
{
    float3 viewPos;
    float pad0;
};

cbuffer ModelData : register(b4)
{
    float4x4 model;
    float4x4 normalMat;

    float3 baseColor;
    uint hasBaseColorTex;

    float metallicFactor;
    float roughnessFactor;
    uint hasMetallicRoughnessTex;
    
    float padding;
};