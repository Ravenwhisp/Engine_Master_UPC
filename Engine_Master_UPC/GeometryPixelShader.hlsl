#include "NewCBuffers.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

struct VertexOutput
{
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

float4 main(VertexOutput input) : SV_Target0
{
    float4 texSample = diffuseTex.Sample(diffuseSamp, input.texCoord);

    if (hasDiffuseTex != 0 && texSample.a < 0.5f)
    {
        discard;
    }

    float3 albedo = (hasDiffuseTex != 0) ? texSample.rgb * diffuseColour : diffuseColour;
    float3 specular = specularColour;
    float smoothness = shininess;

    return float4(albedo, 1.0f);
}

float4 main_specular(VertexOutput input) : SV_Target1
{
    float4 texSample = diffuseTex.Sample(diffuseSamp, input.texCoord);

    float3 specular = specularColour;
    float smoothness = shininess;

    return float4(specular, smoothness);
}

float4 main_normal(VertexOutput input) : SV_Target2
{
    float3 normalVector = normalize(input.normal);
    return float4(normalVector * 0.5f + 0.5f, 1.0f);
}

float4 main_position(VertexOutput input) : SV_Target3
{
    return float4(input.worldPos, 1.0f);
}
