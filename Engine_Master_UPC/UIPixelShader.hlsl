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
    float2 fillData : TEXCOORD2;
    nointerpolation int method : TEXCOORD3;
    nointerpolation int origin : TEXCOORD4;
    float aspectRatio : TEXCOORD5;
    float alpha : TEXCOORD6;
    float4 position : SV_POSITION;
};

float3 LinearToSRGB(float3 color)
{
    color = max(color, 0.0f);
    return pow(color, INV_GAMMA);
}

static const float2 RAD90_CENTERS[4] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

static const float RAD90_OFFSETS[4] =
{
    0.75f,
    0.0f,
    0.25f,
    0.5f
};

static const float2 RAD180_CENTERS[4] =
{
    float2(0.5f, 1.0f),
    float2(0.0f, 0.5f),
    float2(0.5f, 0.0f),
    float2(1.0f, 0.5f)
};

static const float RAD180_OFFSETS[4] =
{
    0.5f,
    0.75f,
    0.0f,
    0.25f
};

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

float ComputeLinearMask(float start, float end, float softness, float fillUV)
{
    float a = smoothstep(end + softness, end - softness, fillUV);
    float b = smoothstep(start + softness, start - softness, fillUV);
    return saturate(a - b);
}

float4 main(PSInput input) : SV_TARGET
{
    float fillStart = saturate(input.fillData.x);
    float fillEnd = saturate(input.fillData.y);
    float2 fillUV = input.fillUV;
    int method = input.method;
    int origin = input.origin;
    float aspectRatio = max(input.aspectRatio, 0.0001f);
    
    if (fillEnd <= 0.0f || fillStart >= fillEnd)
        return 0;

    float mask = 1.0f;
    
    if (method < FILL_RADIAL90)
    {
        float uv = (method == FILL_HORIZONTAL) ? fillUV.x : fillUV.y;

        bool flip =
            (method == FILL_HORIZONTAL && origin != 0) ||
            (method == FILL_VERTICAL && origin == 0);

        if (flip)
        {
            uv = 1.0f - uv;
        }

        float softness = max(fwidth(uv) * 1.5f, 0.001f);
        mask = ComputeLinearMask(fillStart, fillEnd, softness, uv);
    }
    else if (method == FILL_RADIAL90 || method == FILL_RADIAL180)
    {
        float clockwise = ((origin & 4) == 0) ? 1.0f : 0.0f;
        int o = origin & 3;

        float2 center;
        float offset;
        float range;

        if (method == FILL_RADIAL90)
        {

            center = RAD90_CENTERS[o];
            offset = RAD90_OFFSETS[o];
            range = 0.25f;

            if (clockwise < 0.5f)
                offset = 1.0f - offset - range;
        }
        else // FILL_RADIAL180
        {

            center = RAD180_CENTERS[o];
            offset = RAD180_OFFSETS[o];
            range = 0.5f;

            if (clockwise < 0.5f)
                offset = 1.0f - offset - range;
        }

        mask = ComputeRadialMask(fillUV, clockwise, range, offset, center, aspectRatio, fillStart, fillEnd);
    }
    else if (method == FILL_RADIAL360)
    {
        float clockwise = (origin == 0) ? 1.0f : 0.0f;

        mask = ComputeRadialMask(fillUV, clockwise, 1.0f, 0.0f, float2(0.5f, 0.5f), aspectRatio, fillStart, fillEnd);
    }
    
    float4 texColor = uiTexture.Sample(uiSampler, input.texCoord);
    texColor.a *= mask;
    texColor.a *= saturate(input.alpha);
    clip(texColor.a - 0.001f);
    return float4(LinearToSRGB(texColor.rgb), texColor.a);

}