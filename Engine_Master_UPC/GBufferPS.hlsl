#include "GBufferCommon.hlsli"

// Root param [3]: albedo texture (t0) + sampler (s0)
Texture2D diffuseTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
SamplerState gSampler       : register(s0);

struct VSOutput
{
    float4 clipPos  : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
};

struct PSOutput
{
    float4 diffuse        : SV_Target0;   // albedo
    float4 metalRoughness : SV_Target1;   // specular.rgb + smoothness.a
    float4 normal         : SV_Target2;   // world normal
    float4 position       : SV_Target3;   // world position
};

PSOutput main(VSOutput IN)
{
    PSOutput OUT;
    
    // Diffuse
    float4 texSampleD = diffuseTex.Sample(gSampler, IN.texCoord);
    if (gMaterial.hasDiffuseTex != 0 && texSampleD.a < 0.5f)
    {
        discard;
    }
    float3 albedo = (gMaterial.hasDiffuseTex != 0) ? texSampleD.rgb * gMaterial.diffuseColour : gMaterial.diffuseColour;
    OUT.diffuse      = float4(albedo, 1.0f);
    
    // Metallic-roughness
    float minRoughness = 0.04;
    
    float4 texSampleM = metallicRoughnessTex.Sample(gSampler, IN.texCoord);

    float metallic = gMaterial.hasMetallicRoughnessTex != 0 ? 1 - saturate(texSampleM.b * gMaterial.metallicFactor) : gMaterial.metallicFactor;
    metallic = 1 - metallic;
    float perceptualRoughness = gMaterial.hasMetallicRoughnessTex != 0 ? clamp(texSampleM.g * gMaterial.roughnessFactor, minRoughness, 1.0) : gMaterial.roughnessFactor;
    perceptualRoughness = 1 - perceptualRoughness;
    OUT.metalRoughness     = float4(metallic, perceptualRoughness, 0, 0);
    
    // Normal
    OUT.normal       = float4(normalize(IN.normal), 0.0f);
    
    // Position
    OUT.position     = float4(IN.worldPos, 0.0f);

    return OUT;
}