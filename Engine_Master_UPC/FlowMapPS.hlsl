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

Texture2D diffuseTexture : register(t0);
Texture2D metallicRoughnessTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D emissiveTexture : register(t3);
Texture2D flowTexture : register(t13);

SamplerState linearWrapSampler : register(s0);

struct PSOutput
{
    float4 diffuse : SV_Target0;
    float4 metalRoughness : SV_Target1;
    float4 normal : SV_Target2;
    float4 position : SV_Target3;
    float4 emissive : SV_Target4;
};

PSOutput main(float4 position : SV_POSITION, float3 worldPos : TEXCOORD0,
              float2 coord : TEXCOORD1, float3 normal : TEXCOORD2,
              float3 tangent : TEXCOORD3)
{
    PSOutput output;
    float2 flowCoord = coord;
    if (flowEnabled != 0 && flowSource == 1)
    {
        float2 flowVector = flowTexture.Sample(linearWrapSampler, coord).rg * 2.0f - 1.0f;
        flowCoord += flowVector * flowStrength;
    }

    float3 albedo = diffuseColor;
    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    float ao = 1.0f;
    float3 finalNormal = normalize(normal);
    float3 emissive = 0.0f;

    if (hasDiffuseTex != 0)
    {
        float4 sampleColor = diffuseTexture.Sample(linearWrapSampler, flowCoord);
        if (sampleColor.a < 0.5f)
            discard;
        albedo *= sampleColor.rgb;
    }

    if (hasMetallicRoughnessTex != 0)
    {
        float4 sampleM = metallicRoughnessTexture.Sample(linearWrapSampler, flowCoord);
        ao = sampleM.r;
        roughness = 1.0f - clamp(sampleM.g, 0.04f, 1.0f);
        metallic = saturate(sampleM.b * metallicFactor);
    }

    if (hasNormalTex != 0)
    {
        float3 tangentNormal = normalTexture.Sample(linearWrapSampler, flowCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0f - 1.0f);
        float3 tangentVector = normalize(tangent);
        float3 bitangentVector = normalize(cross(finalNormal, tangentVector));
        finalNormal = normalize(mul(tangentNormal, float3x3(tangentVector, bitangentVector, finalNormal)));
    }

    if (hasEmissiveTex != 0)
        emissive = emissiveTexture.Sample(linearWrapSampler, flowCoord).rgb * emissiveColor;

    output.diffuse = float4(albedo, 1.0f);
    output.metalRoughness = float4(ao, roughness, metallic, 0.0f);
    output.normal = float4(finalNormal, 0.0f);
    output.position = float4(worldPos, 0.0f);
    output.emissive = float4(emissive, 0.0f);
    return output;
}
