cbuffer CameraParams : register(b0)
{
    float4x4 vp;
};

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR;
};

VSOut main(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL, float3 tangent : TANGENT, float4 color : COLOR)
{
    VSOut output;
    output.texCoord = texCoord;
    output.position = mul(float4(position, 1.0f), vp);
    output.normal = normal;
    output.tangent = tangent;
    output.color = color;

    
    return output;
}