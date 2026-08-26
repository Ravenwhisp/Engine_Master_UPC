cbuffer MvpCB : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.texCoord = input.texCoord;
    return output;
}