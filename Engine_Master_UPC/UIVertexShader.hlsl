cbuffer UIParams : register(b0)
{
    float4x4 mvp;
};

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float4 position : SV_POSITION;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, float4 color : COLOR)
{
    VSOut output;
    output.texCoord = texCoord;
    output.color = color;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}