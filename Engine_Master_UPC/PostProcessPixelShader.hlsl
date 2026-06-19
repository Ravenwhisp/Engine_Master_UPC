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
    uint  pad0;
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

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    float3 hdr = sampleScene(uv);

    if (enableBloom != 0)
    {
        float3 bloom = bloomTexture.Sample(bilinearClamp, uv).rgb;
        hdr += bloom * bloomIntensity;
    }
    
    hdr *= 1.2 * pow(2.0, exposure);
    
    float3 mapped = PBRNeutralToneMapping(hdr);
    
    if (enableLUT != 0)
        mapped = applyLUT(mapped);
    
    return float4(LinearToSRGB(mapped), 1.0);
}
