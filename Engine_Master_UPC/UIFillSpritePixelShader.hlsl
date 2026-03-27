Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

static const float TWO_PI = 6.28318530f;

#define FILL_HORIZONTAL 0.0f
#define FILL_VERTICAL 1.0f
#define FILL_RADIAL90 2.0f
#define FILL_RADIAL180 3.0f
#define FILL_RADIAL360 4.0f

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

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

float4 main(VertexShaderOutput input) : SV_TARGET
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
            float softness = fwidth(input.texcoord.x) * 1.5f;
            if (origin < 0.5f)
                mask = smoothstep(edge + softness, edge - softness, input.texcoord.x);
            else
                mask = smoothstep(edge + softness, edge - softness, 1.0f - input.texcoord.x);
        }
        else if (method < (FILL_VERTICAL + 0.5f))
        {
            float edge = fillAmount;
            float softness = fwidth(input.texcoord.y) * 1.5f;
            if (origin < 0.5f)
                mask = smoothstep(edge + softness, edge - softness, 1.0f - input.texcoord.y);
            else
                mask = smoothstep(edge + softness, edge - softness, input.texcoord.y);
        }
        else if (method < (FILL_RADIAL90 + 0.5f))
        {
            float2 center = float2(0.0f, 1.0f);
            float offset = 0.75f;
            if (origin < 0.5f) { center = float2(0.0f, 1.0f); offset = 0.75f; } // Bottom Left
            else if (origin < 1.5f) { center = float2(0.0f, 0.0f); offset = 0.0f; } // Top Left
            else if (origin < 2.5f) { center = float2(1.0f, 0.0f); offset = 0.25f; } // Top Right
            else { center = float2(1.0f, 1.0f); offset = 0.5f; } // Bottom Right
            mask = ComputeRadialMask(input.texcoord, fillAmount, 1.0f, 0.25f, offset, center, aspectRatio);
        }
        else if (method < (FILL_RADIAL180 + 0.5f))
        {
            float2 center = float2(0.5f, 1.0f);
            float offset = 0.5f;
            if (origin < 0.5f) { center = float2(0.5f, 1.0f); offset = 0.5f; } // Bottom
            else if (origin < 1.5f) { center = float2(0.0f, 0.5f); offset = 0.75f; } // Left
            else if (origin < 2.5f) { center = float2(0.5f, 0.0f); offset = 0.0f; } // Top
            else { center = float2(1.0f, 0.5f); offset = 0.25f; } // Right
            mask = ComputeRadialMask(input.texcoord, fillAmount, 1.0f, 0.5f, offset, center, aspectRatio);
        }
        else
        {
            float clockwise = (origin < 0.5f) ? 1.0f : 0.0f;
            mask = ComputeRadialMask(input.texcoord, fillAmount, clockwise, 1.0f, 0.0f, float2(0.5f, 0.5f), aspectRatio);
        }
    }

    clip(mask - 0.0001f);
    
    float4 col = spriteTexture.Sample(spriteSampler, input.texcoord);
    col.a *= mask;
    return col;
}