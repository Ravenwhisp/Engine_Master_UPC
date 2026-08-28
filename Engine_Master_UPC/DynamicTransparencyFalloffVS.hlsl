#include "DynamicTransparencyFalloffCommon.hlsli"

cbuffer MvpCB : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.position = mul(float4(input.position, 1.0f), mvp);
    output.worldPos = mul(float4(input.position, 1.0f), model).xyz;
    output.texCoord = input.texCoord;
    output.normal = normalize(mul(input.normal, (float3x3) normalMat));
    output.tangent = normalize(mul(input.tangent, (float3x3) normalMat));

    return output;
}