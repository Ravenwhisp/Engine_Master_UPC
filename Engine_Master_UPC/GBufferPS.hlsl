#include "GBufferCommon.hlsli"

// Root param [3]: albedo texture (t0) + sampler (s0)
Texture2D    gAlbedoTexture : register(t0);
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
    float4 diffuse  : SV_Target0;   // albedo
    float4 specular : SV_Target1;   // specular.rgb + smoothness.a
    float4 normal   : SV_Target2;   // world normal
    float4 position : SV_Target3;   // world position
};

PSOutput main(VSOutput IN)
{
    PSOutput OUT;

    float4 texColour = gAlbedoTexture.Sample(gSampler, IN.texCoord);
    float3 albedo    = texColour.rgb * gMaterial.albedoColor.rgb;

    OUT.diffuse      = float4(albedo, 1.0f);
    OUT.specular     = gMaterial.specularColor;                  // a = smoothness
    OUT.normal       = float4(normalize(IN.normal), 0.0f);
    OUT.position     = float4(IN.worldPos, 0.0f);

    return OUT;
}