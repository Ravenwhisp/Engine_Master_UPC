#define UITextEffect_Outline 1
#define UITextEffect_Shadow  2
#define UITextEffect_Glow    4
#define UITextEffect_Wave    8

Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

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

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

float sampleAlpha(float2 uv)
{
    return fontTexture.Sample(fontSampler, uv).a;
}

float4 main(PSInput input) : SV_Target
{
    float alpha = sampleAlpha(input.texCoord);

    float4 result = input.color * alpha;

    if ((effectFlags & UITextEffect_Outline) != 0)
    {
        float2 d = atlasTexelSize * outlineSize;

        float outlineAlpha = 0.0f;
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(d.x, 0.0f)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(-d.x, 0.0f)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(0.0f, d.y)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(0.0f, -d.y)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(d.x, d.y)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(-d.x, d.y)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(d.x, -d.y)));
        outlineAlpha = max(outlineAlpha, sampleAlpha(input.texCoord + float2(-d.x, -d.y)));

        float border = saturate(outlineAlpha - alpha);
        result = lerp(result, outlineColor, border * outlineColor.a);
        result.a = max(result.a, border * outlineColor.a);
    }

    if ((effectFlags & UITextEffect_Glow) != 0)
    {
        float2 d = atlasTexelSize * glowSize;

        float glowAlpha = 0.0f;
        glowAlpha += sampleAlpha(input.texCoord + float2(d.x, 0.0f));
        glowAlpha += sampleAlpha(input.texCoord + float2(-d.x, 0.0f));
        glowAlpha += sampleAlpha(input.texCoord + float2(0.0f, d.y));
        glowAlpha += sampleAlpha(input.texCoord + float2(0.0f, -d.y));

        glowAlpha += sampleAlpha(input.texCoord + float2(d.x, d.y));
        glowAlpha += sampleAlpha(input.texCoord + float2(-d.x, d.y));
        glowAlpha += sampleAlpha(input.texCoord + float2(d.x, -d.y));
        glowAlpha += sampleAlpha(input.texCoord + float2(-d.x, -d.y));

        glowAlpha = saturate(glowAlpha * 0.25f);

        result.rgb += glowColor.rgb * glowAlpha * glowColor.a;
        result.a = max(result.a, glowAlpha * glowColor.a);
    }

    return result;
}