cbuffer Mvp : register(b0)
{
    float4x4 mvp;
};

cbuffer ModelDataCB : register(b4)
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

    float3 padding;
};



Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D positionTex : register(t3);
Texture2D emissiveTex : register(t4);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);



float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float3 tangent : TANGENT, float2 coord : TEXCOORD, float4 position : SV_POSITION) : SV_TARGET
{
    float3 finalWorldNormal = normalize(normal);
    
    
    //Load normal texture
    if (hasNormalTex != 0)
    {
        float3 tangentNormal = normalTex.Sample(linearWrapSample, coord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
        
        float3 tangentVector = normalize(tangent.xyz);
        float3 bitangentVector = cross(finalWorldNormal, tangentVector);
        float3x3 TBN = float3x3(tangentVector, bitangentVector, finalWorldNormal);
    
        finalWorldNormal = mul(tangentNormal, TBN);
    }
    
    return float4(finalWorldNormal, 1.0f);
}