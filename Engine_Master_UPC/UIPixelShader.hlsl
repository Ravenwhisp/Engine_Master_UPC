Texture2D uiTexture : register(t0);
SamplerState uiSampler : register(s0);

static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;
static const float TWO_PI = 6.28318530f;

#define FILL_HORIZONTAL 0.0f
#define FILL_VERTICAL 1.0f
#define FILL_RADIAL90 2.0f
#define FILL_RADIAL180 3.0f
#define FILL_RADIAL360 4.0f

struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float4 position : SV_POSITION;
};

float3 LinearToSRGB(float3 color)
{
    color = max(color, 0.0f);
    return pow(color, INV_GAMMA);
}

float ComputeRadialMask(float2 uv, float fillAmount, float clockwise, float range, float offset, float2 center, float aspectRatio)
{
    if (fillAmount <= 0.0f)
        return 0.0f;

    float2 dir = uv - center;
    dir.x *= aspectRatio;
    float angle = atan2(dir.y, dir.x);
    float angleNorm = (angle < 0.0f ? angle + TWO_PI : angle) / TWO_PI;

    if (clockwise < 0.5f)
        angleNorm = 1.0f - angleNorm;

    angleNorm = frac(angleNorm - offset + 1.0f);

    if (angleNorm > range)
        return 0.0f;

    float edge = fillAmount * range;
    float softness = fwidth(angleNorm) * 1.5f;
    return smoothstep(edge + softness, edge - softness, angleNorm);
}

float4 main(PSInput input) : SV_TARGET
{
    float fillAmount = saturate(input.color.r);
    float method = input.color.g;
    float origin = input.color.b;
    float aspectRatio = max(input.color.a, 0.0001f);

    float mask = 1.0f;

    if (fillAmount < 1.0f)
    {
        if (method < (FILL_HORIZONTAL + 0.5f))
        {
            float edge = fillAmount;
            float softness = fwidth(input.texCoord.x) * 1.5f;
            if (origin < 0.5f)
                mask = smoothstep(edge + softness, edge - softness, input.texCoord.x);
            else
                mask = smoothstep(edge + softness, edge - softness, 1.0f - input.texCoord.x);
        }
        else if (method < (FILL_VERTICAL + 0.5f))
        {
            float edge = fillAmount;
            float softness = fwidth(input.texCoord.y) * 1.5f;
            if (origin < 0.5f)
                mask = smoothstep(edge + softness, edge - softness, 1.0f - input.texCoord.y);
            else
                mask = smoothstep(edge + softness, edge - softness, input.texCoord.y);
        }
        else if (method < (FILL_RADIAL90 + 0.5f))
        {
            float2 center = float2(0.0f, 1.0f);
            float offset = 0.75f;
            if (origin < 0.5f) { center = float2(0.0f, 1.0f); offset = 0.75f; }
            else if (origin < 1.5f) { center = float2(0.0f, 0.0f); offset = 0.0f; }
            else if (origin < 2.5f) { center = float2(1.0f, 0.0f); offset = 0.25f; }
            else { center = float2(1.0f, 1.0f); offset = 0.5f; }
            mask = ComputeRadialMask(input.texCoord, fillAmount, 1.0f, 0.25f, offset, center, aspectRatio);
        }
        else if (method < (FILL_RADIAL180 + 0.5f))
        {
            float2 center = float2(0.5f, 1.0f);
            float offset = 0.5f;
            if (origin < 0.5f) { center = float2(0.5f, 1.0f); offset = 0.5f; }
            else if (origin < 1.5f) { center = float2(0.0f, 0.5f); offset = 0.75f; }
            else if (origin < 2.5f) { center = float2(0.5f, 0.0f); offset = 0.0f; }
            else { center = float2(1.0f, 0.5f); offset = 0.25f; }
            mask = ComputeRadialMask(input.texCoord, fillAmount, 1.0f, 0.5f, offset, center, aspectRatio);
        }
        else
        {
            float clockwise = (origin < 0.5f) ? 1.0f : 0.0f;
            mask = ComputeRadialMask(input.texCoord, fillAmount, clockwise, 1.0f, 0.0f, float2(0.5f, 0.5f), aspectRatio);
        }
    }

    clip(mask - 0.0001f);

    float4 texColor = uiTexture.Sample(uiSampler, input.texCoord);
    texColor.a *= mask;
    return float4(LinearToSRGB(texColor.rgb), texColor.a);
}