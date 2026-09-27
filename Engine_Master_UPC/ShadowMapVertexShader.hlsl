#define SHADOW_CB_REGISTER b1
#include "ShadowData.hlsli"
cbuffer ShadowDrawData : register(b0) { float4x4 model; };
cbuffer ShadowRenderParams : register(b2) { uint shadowMatrixIndex; };
struct ShadowVertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
ShadowVertexOutput main(float3 position : POSITION, float2 uv : TEXCOORD0)
{
    ShadowVertexOutput output;
    output.position = mul(mul(float4(position, 1), model), cascadeLightViewProjection[shadowMatrixIndex]);
    output.uv = uv;
    return output;
}
