cbuffer UIParams : register(b0)
{
    float4x4 mvp;
    float4 fillData;
    float alpha;
    float3 _pad;
};

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 fillData : COLOR0;
    float alpha : TEXCOORD1;
    float4 position : SV_POSITION;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, float4 color : COLOR)
{
    VSOut output;
    output.texCoord = texCoord;
    output.fillData = fillData;
    output.alpha = alpha;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}