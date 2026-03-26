cbuffer SpriteBatchConstants : register(b0)
{
    row_major float4x4 transform;
};

struct VertexShaderInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, transform);
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}
