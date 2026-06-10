cbuffer UIParams : register(b0)
{
    float4x4 mvp;
    float2 fillData;
    float2 sheetOffset;
    float2 uvScale;
    int method;
    int origin;
    float aspectRatio;
    float alpha;
    float2 padding;
};

struct VSOut
{
    float2 texCoord : TEXCOORD0;
    float2 fillUV : TEXCOORD1;
    float2 fillData : TEXCOORD2;
    nointerpolation int method : TEXCOORD3;
    nointerpolation int origin : TEXCOORD4;
    float aspectRatio : TEXCOORD5;
    float alpha : TEXCOORD6;
    float4 position : SV_POSITION;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, float4 color : COLOR)
{
    VSOut output;
    output.fillUV = texCoord;
        
    output.texCoord = (texCoord - 0.5f) * uvScale + sheetOffset;
    output.fillData = fillData;
    output.method = method;
    output.origin = origin;
    output.aspectRatio = aspectRatio;
    output.alpha = alpha;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}