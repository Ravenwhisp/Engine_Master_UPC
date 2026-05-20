cbuffer UIParams : register(b0)
{
    float4x4 mvp;
    float4 fillData;
    float alpha;
    float sheetColumns;
    float sheetRows;
    float _pad0;
    float2 sheetOffset;
    float2 uvScale;
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
    float2 scaledUv = texCoord * uvScale;
    float2 tile = float2(max(sheetColumns, 1.0f), max(sheetRows, 1.0f));

    output.texCoord = scaledUv / tile + sheetOffset;
    output.fillData = fillData;
    output.alpha = alpha;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}