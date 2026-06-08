cbuffer UIParams : register(b0)
{
    float4x4 mvp;
    float4 fillData;
    float2 sheetOffset;
    float2 uvScale;
    float aspectRatio;
    float alpha;
    float2 padding;
};

struct VSOut
{
    float2 texCoord : TEXCOORD0;
    float2 fillUV : TEXCOORD1;
    float4 fillData : COLOR0;
    float aspectRatio : TEXCOORD2;
    float alpha : TEXCOORD3;
    float4 position : SV_POSITION;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, float4 color : COLOR)
{
    VSOut output;
    output.fillUV = texCoord;
        
    output.texCoord = (texCoord - 0.5f) * uvScale + sheetOffset;
    output.fillData = fillData;
    output.aspectRatio = aspectRatio;
    output.alpha = alpha;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}