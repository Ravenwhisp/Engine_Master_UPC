#include "General.hlsli"    
#include "PBRGeneral.hlsli" 

Texture2D    sceneTexture  : register(t0);
Texture2D    bloomTexture  : register(t1);
Texture3D    lutTexture    : register(t2);
Texture2D    depthTexture  : register(t3);
SamplerState bilinearClamp : register(s0);

cbuffer PostProcessParams : register(b0)
{
    float exposure;
    float bloomIntensity;
    float lutSize;
    float caStrength;

    uint  enableBloom;
    uint  enableLUT;
    uint  enableCA;
    uint  enableHeartbeat;

    // Heartbeat / damage screen effect (computed on the CPU each frame).
    float hbHealthVignette;
    float hbSepVignette;
    float hbPulse;
    uint  hbPulseIsLub;

    float hbCrit;
    float hbDesat;
    float hbSwayX;
    float hbSwayY;

    // Death fade.
    float deathDesat;
    float deathFade;
    float deathBlur;
    float deathPad0;

    // Outline (ink).
    uint  enableOutline;
    float outlineThickness;
    float outlineThreshold;
    float outlineIntensity;

    float outlineColorR;
    float outlineColorG;
    float outlineColorB;
    float outlineWobble;

    float outlineNoiseScale;
    float outlineBreakup;
    float outlinePad0;
    float outlinePad1;
};

float hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float valueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float3 applyOutline(float3 color, float2 uv)
{
    float2 texSize;
    sceneTexture.GetDimensions(texSize.x, texSize.y);
    float2 texel = 1.0 / texSize;
    float2 o = texel * outlineThickness;
    
    float2 warp = float2(valueNoise(uv * outlineNoiseScale),
                         valueNoise(uv * outlineNoiseScale + 17.0)) - 0.5;
    float2 suv = uv + warp * texel * outlineThickness * outlineWobble * 2.0;
    
    float d0 = depthTexture.Sample(bilinearClamp, suv - o).r;
    float d1 = depthTexture.Sample(bilinearClamp, suv + o).r;
    float d2 = depthTexture.Sample(bilinearClamp, suv + float2(o.x, -o.y)).r;
    float d3 = depthTexture.Sample(bilinearClamp, suv + float2(-o.x, o.y)).r;
    float dc = depthTexture.Sample(bilinearClamp, suv).r;

    float g = abs(d0 - d1) + abs(d2 - d3);
    
    float edge = (dc < 0.9999) ? g : 0.0;
    edge = smoothstep(outlineThreshold, outlineThreshold * 3.0 + 1e-4, edge);
    
    float breakup = lerp(1.0, valueNoise(uv * outlineNoiseScale * 2.3), outlineBreakup);
    edge *= breakup;

    float3 ink = float3(outlineColorR, outlineColorG, outlineColorB);
    return lerp(color, ink, saturate(edge * outlineIntensity));
}

float3 sampleScene(float2 uv)
{
    if (enableCA == 0)
        return sceneTexture.Sample(bilinearClamp, uv).rgb;
    
    const float3 colourOffset = float3(0.015, 0.008, -0.008) * caStrength;
    float2 dir = float2(0.5, 0.5) - uv;

    float3 c;
    c.r = sceneTexture.Sample(bilinearClamp, uv + dir * colourOffset.r).r;
    c.g = sceneTexture.Sample(bilinearClamp, uv + dir * colourOffset.g).g;
    c.b = sceneTexture.Sample(bilinearClamp, uv + dir * colourOffset.b).b;
    return c;
}

float3 applyLUT(float3 color)
{
    float3 uvw = (saturate(color) * (lutSize - 1.0) + 0.5) / lutSize;
    return lutTexture.SampleLevel(bilinearClamp, uvw, 0).rgb;
}

float3 sampleSceneBlurred(float2 uv, float blur)
{
    if (blur <= 0.001)
        return sampleScene(uv);

    const int   N = 64;
    const float goldenAngle = 2.39996323;
    const float maxRadius = blur * 0.006;

    float2 texSize;
    sceneTexture.GetDimensions(texSize.x, texSize.y);
    float aspect = texSize.y / texSize.x; 

    float3 sum = 0.0;
    float  totalWeight = 0.0;

    [unroll]
    for (int i = 0; i < N; ++i)
    {
        float t = (float(i) + 0.5) / float(N);
        float r = sqrt(t) * maxRadius;         
        float a = float(i) * goldenAngle;
        float2 offset = float2(cos(a) * aspect, sin(a)) * r;
        float  w = exp(-1.5 * t);                 

        // Clamp so one very bright pixel can't ghost into bright copies.
        float3 s = min(sceneTexture.Sample(bilinearClamp, uv + offset).rgb, 8.0);
        sum += s * w;
        totalWeight += w;
    }

    return sum / totalWeight;
}

// Layered low-health "damage screen": desaturation, red/blue vignettes,
// heartbeat pulse tint and critical edge darkening.
float3 applyHeartbeat(float3 color, float2 uv)
{
    float lum = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(color, lum.xxx, saturate(hbDesat));
    
    float r = saturate(length(uv - 0.5) * 1.4);
    float edge = smoothstep(0.3, 1.0, r);
    
    float hv = edge * hbHealthVignette;
    color = lerp(color, float3(0.5, 0.0, 0.0), hv);
    color *= 1.0 - hv * 0.5;
    
    float sv = edge * hbSepVignette;
    color = lerp(color, float3(0.0, 0.1, 0.4), sv);
    
    float3 pulseCol = hbPulseIsLub ? float3(1.0, 0.78, 0.70) : float3(0.70, 0.86, 1.0);
    color += pulseCol * (hbPulse * (hbPulseIsLub ? 0.25 : 0.18));
    
    color *= 1.0 - edge * hbCrit;

    return color;
}

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    // Heartbeat sway shifts the scene sampling
    float2 suv = uv + float2(hbSwayX, hbSwayY);

    float sceneDepth = depthTexture.Sample(bilinearClamp, uv).r;
    bool isBackground = sceneDepth >= 0.9999;

    float3 outColor;
    if (isBackground)
    {
        outColor = saturate(sampleScene(suv));
    }
    else
    {
        // Scene sample, blurred out of focus during the death fade.
        float3 hdr = sampleSceneBlurred(suv, deathBlur);

        if (enableBloom != 0)
        {
            float3 bloom = min(bloomTexture.Sample(bilinearClamp, suv).rgb, 65000.0);
            hdr += bloom * bloomIntensity;
        }

        hdr *= 1.2 * pow(2.0, exposure);

        float3 mapped = PBRNeutralToneMapping(hdr);

        if (enableLUT != 0)
            mapped = applyLUT(mapped);

        outColor = LinearToSRGB(mapped);
    }
    
    if (enableOutline != 0)
        outColor = applyOutline(outColor, uv);

    if (enableHeartbeat != 0)
        outColor = applyHeartbeat(outColor, uv);

    // Death fade: desaturate to grey, then fade fully to black.
    float deathLum = dot(outColor, float3(0.299, 0.587, 0.114));
    outColor = lerp(outColor, deathLum.xxx, saturate(deathDesat));
    outColor *= 1.0 - saturate(deathFade);

    return float4(outColor, 1.0);
}
