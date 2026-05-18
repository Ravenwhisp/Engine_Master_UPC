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
    float4x4 nm;
};
 
VertexOutput main(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL, float3 tangent : TANGENT)
{
    VertexOutput output;
    output.worldPos = mul(float4(position, 1.0), model).xyz;
    
    //output.normal = normalize(mul(normal, (float3x3) normalMat));
    float3 normalVec = normalize(mul(normal, (float3x3)normalMat));
    output.normal = mul(float4(normalVec, 1), nm);
    
    //output.tangent = normalize(tangent);
    float3 tangentVec = normalize(tangent);
    output.tangent = mul(float4(tangentVec, 1), nm);
    
    output.texCoord = texCoord;
    output.position = mul(float4(position, 1.0f), mvp);
 
    return output;
}