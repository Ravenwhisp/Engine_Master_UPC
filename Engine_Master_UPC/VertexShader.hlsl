#include "NewCBuffers.hlsli"

struct VertexOutput
{
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};
 
VertexOutput main(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL, float3 tangent : TANGENT)
{
    VertexOutput output;
    output.worldPos = mul(float4(position, 1.0), model).xyz;
    
    float3 normalVec = normalize(mul(normal, (float3x3)normalMat));
    output.normal = mul(float4(normalVec, 1), mvp);
    
    float3 tangentVec = normalize(tangent);
    output.tangent = mul(float4(tangentVec, 1), mvp);
    
    output.texCoord = texCoord;
    output.position = mul(float4(position, 1.0f), mvp);
 
    return output;
}