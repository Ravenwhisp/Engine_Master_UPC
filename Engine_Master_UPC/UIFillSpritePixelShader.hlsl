Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

static const float TWO_PI = 6.28318530f;

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

float ComputeRadialMask(float2 uv, float fillAmount, float clockwise, float range)
{
    if (fillAmount <= 0.0f)
        return 0.0f;

    float2 dir = uv - float2(0.5f, 0.5f);
    float angle = atan2(dir.y, dir.x);
    float angleNorm = (angle < 0.0f ? angle + TWO_PI : angle) / TWO_PI;

    if (clockwise < 0.5f)
        angleNorm = 1.0f - angleNorm;

    if (angleNorm > range)
        return 0.0f;

    return angleNorm <= (fillAmount * range) ? 1.0f : 0.0f;
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float fillAmount = saturate(input.color.r);
    float method = input.color.g;
    float clockwise = input.color.b;

    float mask = 1.0f;

    if (fillAmount < 1.0f)
    {
        if (method < 0.5f)
            mask = (input.texcoord.x <= fillAmount) ? 1.0f : 0.0f;
        else if (method < 1.5f)
            mask = (input.texcoord.y >= (1.0f - fillAmount)) ? 1.0f : 0.0f;
        else if (method < 2.5f)
            mask = ComputeRadialMask(input.texcoord, fillAmount, clockwise, 0.25f);
        else if (method < 3.5f)
            mask = ComputeRadialMask(input.texcoord, fillAmount, clockwise, 0.5f);
        else
            mask = ComputeRadialMask(input.texcoord, fillAmount, clockwise, 1.0f);
    }

    if (mask < 0.001f)
        return float4(0, 0, 0, 0);

    return spriteTexture.Sample(spriteSampler, input.texcoord);
}