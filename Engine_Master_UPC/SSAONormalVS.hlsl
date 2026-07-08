struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 viewNormal : NORMAL;
};

cbuffer Transforms : register(b0)
{
    float4x4 mvp;
    float4x4 normalToView;
};

VertexOutput main(
    float3 position : POSITION,
    float2 texCoord : TEXCOORD,
    float3 normal : NORMAL,
    float3 tangent : TANGENT)
{
    VertexOutput output;

    output.position = mul(float4(position, 1.0f), mvp);
    output.viewNormal = normalize(mul(normal, (float3x3) normalToView));

    return output;
}