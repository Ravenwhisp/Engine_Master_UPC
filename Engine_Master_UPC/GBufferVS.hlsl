#include "GBufferCommon.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 clipPos  : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;   // world-space
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    // World position (gModel is pre-transposed: pos * M == mul(pos, M) in HLSL)
    float4 worldPos4 = mul(float4(IN.position, 1.0f), gModel);
    OUT.worldPos     = worldPos4.xyz;

    // Clip position via pre-transposed MVP
    OUT.clipPos      = mul(float4(IN.position, 1.0f), gMVP);

    // World normal (gNormalMat = inverse-transpose world, pre-transposed)
    OUT.normal       = normalize(mul(float4(IN.normal, 0.0f), gNormalMat).xyz);

    OUT.texCoord     = IN.texCoord;
    return OUT;
}