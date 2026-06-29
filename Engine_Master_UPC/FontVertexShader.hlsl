cbuffer FontParams : register(b0)
{
    float2 viewportSize;
    float time;
    float padding;
};

struct VSInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput output;

    float2 ndc;
    ndc.x = (input.position.x / viewportSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (input.position.y / viewportSize.y) * 2.0f;

    output.position = float4(ndc, 0.0f, 1.0f);
    output.texCoord = input.texCoord;
    output.color = input.color;

    return output;
}