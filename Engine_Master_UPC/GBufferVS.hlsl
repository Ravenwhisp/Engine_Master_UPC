#include "GBufferCommon.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
};

struct VSOutput
{
    float4 clipPos  : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;   // world-space
    float3 tangent  : TEXCOORD3;   // world-space
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    
    

    // World position (gModel is pre-transposed: pos * M == mul(pos, M) in HLSL)
    OUT.worldPos = mul(float4(IN.position, 1.0f), gModel).xyz;

    // Clip position via pre-transposed MVP
    OUT.clipPos      = mul(float4(IN.position, 1.0f), gMVP);

    // World normal (gNormalMat = inverse-transpose world, pre-transposed)
    OUT.normal = normalize(mul(IN.normal, (float3x3) gNormalMat));
    
    // World tangent (gNormalMat = inverse-transpose world, pre-transposed)
    OUT.tangent = normalize(mul(IN.tangent, (float3x3) gNormalMat));
    
    OUT.texCoord     = IN.texCoord;
    
    return OUT;
}