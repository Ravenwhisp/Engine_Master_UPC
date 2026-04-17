cbuffer SpriteParams : register(b0)
{
    float4x4 mvp;
};

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float4 position : SV_POSITION;
};

VSOut main(float3 position : POSITION, float2 texCoord : TEXCOORD)
{
    VSOut output;
    output.texCoord = texCoord;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    output.position = mul(float4(position, 1.0f), mvp);
    return output;
}