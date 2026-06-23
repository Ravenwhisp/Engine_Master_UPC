#include "General.hlsli"    
#include "PBRGeneral.hlsli" 

Texture2D    sceneTexture  : register(t0);
Texture2D    bloomTexture  : register(t1);
Texture3D    lutTexture    : register(t2);
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
};

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

// Out-of-focus disc blur of the scene, used by the death fade. 17 taps in two
// rings; radius grows with the blur amount.
float3 sampleSceneBlurred(float2 uv, float blur)
{
    if (blur <= 0.001)
        return sampleScene(uv);

    const float2 dirs[8] =
    {
        float2( 1.0, 0.0), float2(-1.0, 0.0), float2(0.0, 1.0), float2(0.0, -1.0),
        float2( 0.707, 0.707), float2(-0.707, 0.707), float2(0.707, -0.707), float2(-0.707, -0.707)
    };

    float2 radius = blur * 0.02; // up to ~2% of the screen
    float3 sum = sceneTexture.Sample(bilinearClamp, uv).rgb;

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        sum += sceneTexture.Sample(bilinearClamp, uv + dirs[i] * radius).rgb;
        sum += sceneTexture.Sample(bilinearClamp, uv + dirs[i] * radius * 0.5).rgb;
    }

    return sum / 17.0;
}

// Layered low-health "damage screen": desaturation, red/blue vignettes,
// heartbeat pulse tint and critical edge darkening. Applied to the final
// displayed colour (after tone mapping + gamma).
float3 applyHeartbeat(float3 color, float2 uv)
{
    // Desaturate towards luminance.
    float lum = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(color, lum.xxx, saturate(hbDesat));

    // Radial distance from the screen centre (0 centre .. ~1 corner).
    float r = saturate(length(uv - 0.5) * 1.4);
    float edge = smoothstep(0.3, 1.0, r);

    // Health vignette (red): tint + darken the edges.
    float hv = edge * hbHealthVignette;
    color = lerp(color, float3(0.5, 0.0, 0.0), hv);
    color *= 1.0 - hv * 0.5;

    // Separation vignette (blue).
    float sv = edge * hbSepVignette;
    color = lerp(color, float3(0.0, 0.1, 0.4), sv);

    // Heartbeat pulse: warm flash on the "lub", cooler on the "dub".
    float3 pulseCol = hbPulseIsLub ? float3(1.0, 0.78, 0.70) : float3(0.70, 0.86, 1.0);
    color += pulseCol * (hbPulse * (hbPulseIsLub ? 0.25 : 0.18));

    // Critical edge darkening.
    color *= 1.0 - edge * hbCrit;

    return color;
}

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    // Heartbeat sway shifts the scene sampling (zero when the effect is off).
    float2 suv = uv + float2(hbSwayX, hbSwayY);

    // Scene sample, blurred out of focus during the death fade.
    float3 hdr = sampleSceneBlurred(suv, deathBlur);

    if (enableBloom != 0)
    {
        float3 bloom = bloomTexture.Sample(bilinearClamp, suv).rgb;
        hdr += bloom * bloomIntensity;
    }

    hdr *= 1.2 * pow(2.0, exposure);

    float3 mapped = PBRNeutralToneMapping(hdr);

    if (enableLUT != 0)
        mapped = applyLUT(mapped);

    float3 outColor = LinearToSRGB(mapped);

    if (enableHeartbeat != 0)
        outColor = applyHeartbeat(outColor, uv);

    // Death fade: desaturate to grey, then fade fully to black (no-op when both 0).
    float deathLum = dot(outColor, float3(0.299, 0.587, 0.114));
    outColor = lerp(outColor, deathLum.xxx, saturate(deathDesat));
    outColor *= 1.0 - saturate(deathFade);

    return float4(outColor, 1.0);
}
