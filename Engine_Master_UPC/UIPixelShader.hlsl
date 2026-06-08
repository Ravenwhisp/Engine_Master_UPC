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
    float2 texCoord : TEXCOORD0;
    float2 fillUV : TEXCOORD1;
    float4 fillData : COLOR0;
    float aspectRatio :TEXCOORD2;
    float alpha : TEXCOORD3;
    float4 position : SV_POSITION;
};

float3 LinearToSRGB(float3 color)
{
    color = max(color, 0.0f);
    return pow(color, INV_GAMMA);
}

float ComputeRadialMask(float2 uv, float clockwise, float range, float offset, float2 center, float aspectRatio, float start, float end)
{
    if (end <= start)
        return 0.0f;
    
    if (start <= 0.0f && end >= 1.0f)
        return 1.0f;

    float2 dir = uv - center;
    dir.x *= aspectRatio;
    float angle = atan2(dir.y, dir.x);
    float angleNorm = (angle < 0.0f ? angle + TWO_PI : angle) / TWO_PI;

    if (clockwise < 0.5f)
        angleNorm = 1.0f - angleNorm;

    angleNorm = frac(angleNorm - offset + 1.0f);

    if (angleNorm > range)
        return 0.0f;

    float edgeStart = start * range;
    float edgeEnd = end * range;
    float softness = max(fwidth(angleNorm) * 1.5f, 0.001f);
    float a = smoothstep(edgeEnd + softness, edgeEnd - softness, angleNorm);
    float b = (edgeStart > softness)
        ? smoothstep(edgeStart + softness, edgeStart - softness, angleNorm)
        : 0.0f;
    return saturate(a - b);
}

float4 main(PSInput input) : SV_TARGET
{
    float fillStart = saturate(input.fillData.r);
    float fillEnd = saturate(input.fillData.g);
    float method = input.fillData.b;
    float origin = input.fillData.a;
    float aspectRatio = max(input.aspectRatio, 0.0001f);
    
    if (fillEnd <= 0.0f || fillStart >= fillEnd)
        return 0;

    float mask = 1.0f;
    
    if (method < (FILL_HORIZONTAL + 0.5f))
    {
        float softness = max(fwidth(input.fillUV.x) * 1.5f, 0.001f);
        int o = (int)(origin + 0.5f);
        if (o == 0)
        {
            float a = smoothstep(fillEnd + softness, fillEnd - softness, input.fillUV.x);
            float b = smoothstep(fillStart + softness, fillStart - softness, input.fillUV.x);
            mask = saturate(a - b);
        }
        else
        {
            float x = 1.0f - input.fillUV.x;
            float a = smoothstep(fillEnd + softness, fillEnd - softness, x);
            float b = smoothstep(fillStart + softness, fillStart - softness, x);
            mask = saturate(a - b);
        }
    }
    else if (method < (FILL_VERTICAL + 0.5f))
    {
        float softness = max(fwidth(input.fillUV.y) * 1.5f, 0.001f);
        int o = (int)(origin + 0.5f);
        if (o == 0)
        {
            float t = 1.0f - input.fillUV.y;
            float a = smoothstep(fillEnd + softness, fillEnd - softness, t);
            float b = smoothstep(fillStart + softness, fillStart - softness, t);
            mask = saturate(a - b);
        }
        else
        {
            float t = input.fillUV.y;
            float a = smoothstep(fillEnd + softness, fillEnd - softness, t);
            float b = smoothstep(fillStart + softness, fillStart - softness, t);
            mask = saturate(a - b);
        }
    }
    else if (method < (FILL_RADIAL90 + 0.5f))
    {
        int o = (int)(origin + 0.5f);
        float clockwise = ((o & 4) == 0) ? 1.0f : 0.0f;
        o &= 3;
        float2 center = float2(0.0f, 1.0f);
        float offset = 0.75f;
        if (o == 0) { center = float2(0.0f, 1.0f); offset = 0.75f; }
        else if (o == 1) { center = float2(0.0f, 0.0f); offset = 0.0f; }
        else if (o == 2) { center = float2(1.0f, 0.0f); offset = 0.25f; }
        else { center = float2(1.0f, 1.0f); offset = 0.5f; }
        if (clockwise < 0.5f)
        {
            offset = 1.0f - offset - 0.25f;
        }
        mask = ComputeRadialMask(input.fillUV, clockwise, 0.25f, offset, center, aspectRatio, fillStart, fillEnd);
    }
    else if (method < (FILL_RADIAL180 + 0.5f))
    {
        int o = (int)(origin + 0.5f);
        float clockwise = ((o & 4) == 0) ? 1.0f : 0.0f;
        o &= 3;
        float2 center = float2(0.5f, 1.0f);
        float offset = 0.5f;
        if (o == 0) { center = float2(0.5f, 1.0f); offset = 0.5f; }
        else if (o == 1) { center = float2(0.0f, 0.5f); offset = 0.75f; }
        else if (o == 2) { center = float2(0.5f, 0.0f); offset = 0.0f; }
        else { center = float2(1.0f, 0.5f); offset = 0.25f; }
        if (clockwise < 0.5f)
        {
            offset = 1.0f - offset - 0.5f;
        }
        mask = ComputeRadialMask(input.fillUV, clockwise, 0.5f, offset, center, aspectRatio, fillStart, fillEnd);
    }
    else
    {
        int o = (int)(origin + 0.5f);
        float clockwise = (o == 0) ? 1.0f : 0.0f;
        mask = ComputeRadialMask(input.fillUV, clockwise, 1.0f, 0.0f, float2(0.5f, 0.5f), aspectRatio, fillStart, fillEnd);
    }
    
    float4 texColor = uiTexture.Sample(uiSampler, input.texCoord);
    texColor.a *= mask;
    texColor.a *= saturate(input.alpha);
    clip(texColor.a - 0.001f);
    return float4(LinearToSRGB(texColor.rgb), texColor.a);

}