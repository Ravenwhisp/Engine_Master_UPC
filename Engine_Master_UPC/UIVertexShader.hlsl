cbuffer UIParams : register(b0)
{
    float4x4 mvp;
    float4 fillData;
    float alpha;
    float sheetColumns;
    float sheetRows;
    float flipY;
    float2 sheetOffset;
    float2 uvScale;
};

struct VSOut
{
    float2 texCoord : TEXCOORD0;
    float2 fillUV : TEXCOORD1;
    float4 fillData : COLOR0;
    float alpha : TEXCOORD2;
    float4 position : SV_POSITION;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, float4 color : COLOR)
{
    VSOut output;
    output.fillUV = texCoord;
        
    float2 scaledUv = (texCoord - 0.5f) * uvScale + 0.5f + floor(uvScale);
    float2 tile = float2(max(sheetColumns, 1.0f), max(sheetRows, 1.0f));
    if (flipY > 0.5f)
    {
        scaledUv.y = 1.0f - scaledUv.y;
    }
    else
    {
        scaledUv.x = 1.0f - scaledUv.x;
    }
    output.texCoord = scaledUv / tile + sheetOffset;
    output.fillData = fillData;
    output.alpha = alpha;
    output.position = mul(float4(position, 0.0f, 1.0f), mvp);
    return output;
}