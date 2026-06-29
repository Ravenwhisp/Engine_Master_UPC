#define UITextEffect_Outline 1
#define UITextEffect_Shadow  2
#define UITextEffect_Glow    4
#define UITextEffect_Wave    8

cbuffer FontParams : register(b0)
{
    float2 viewportSize;
    float2 atlasTexelSize;

    float time;
    uint effectFlags;
    float outlineSize;
    float glowSize;

    float4 outlineColor;
    float4 glowColor;

    float waveAmplitude;
    float waveFrequency;
    float waveSpeed;
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

    float2 pos = input.position;

    if ((effectFlags & UITextEffect_Wave) != 0)
    {
        pos.y += sin(pos.x * waveFrequency + time * waveSpeed) * waveAmplitude;
    }

    float2 ndc;
    ndc.x = (pos.x / viewportSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (pos.y / viewportSize.y) * 2.0f;

    output.position = float4(ndc, 0.0f, 1.0f);
    output.texCoord = input.texCoord;
    output.color = input.color;

    return output;
}