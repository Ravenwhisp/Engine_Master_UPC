#include "GBufferCommon.hlsli"
#include "General.hlsli"

struct DamageHighlightData
{
    float  damageHighlight;
    float3 damageHighlightCenterColor;
    float3 damageHighlightRimColor;
    float  damageHighlightRimIntensity;
};

struct DamageHighlightCB
{
    uint hasDamageHighlightComponent;
    float3 padding1;
    
    DamageHighlightData damageHighlightData;
};

struct DissolveData
{
    float  dissolveAmount;
    float3 dissolveColor;
    float  dissolveThickness;
};

struct DissolveCB
{
    uint hasDissolveComponent;
    float3 padding1;
    
    DissolveData dissolveData;
    float3 padding2;
};

cbuffer VisualEffectsCB : register(b4)
{
    DamageHighlightCB damageHighlightCB;
    
    DissolveCB dissolveCB;
};



// Root param [4]: albedo texture (t0) + sampler (s0)
Texture2D diffuseTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D emissiveTex : register(t3);

Texture2D dissolveNoise : register(t8);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);

struct VSOutput
{
    float4 clipPos  : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float3 tangent  : TEXCOORD3;
};

struct PSOutput
{
    float4 diffuse        : SV_Target0;   // albedo
    float4 metalRoughness : SV_Target1;   // specular.rgb + smoothness.a
    float4 normal         : SV_Target2;   // world normal
    float4 position       : SV_Target3;   // world position
    float4 emissive       : SV_Target4;   // emissive
};



float3 CalculateDamageHighlight(float3 normalVector, float3 viewDirection, float3 albedo)
{
    float NdotV = saturate(dot(normalVector, viewDirection));
    
    float3 fresnelColor = ColoredSchlickFresnel(damageHighlightCB.damageHighlightData.damageHighlightCenterColor * albedo, damageHighlightCB.damageHighlightData.damageHighlightRimColor, NdotV, 25 - damageHighlightCB.damageHighlightData.damageHighlightRimIntensity);
    
    return fresnelColor * damageHighlightCB.damageHighlightData.damageHighlight;
}



PSOutput main(VSOutput IN)
{
    PSOutput OUT;
    
    
    float metallic = gMaterial.metallicFactor;
    float alphaRoughness = gMaterial.roughnessFactor;
    float minRoughness = 0.04;
    float ao = 1;
    float3 emissive = 0;
    float3 finalWorldNormal = normalize(IN.normal);
    
    bool dissolving = false;
    
    
    
    //Load base color texture
    float3 albedo = gMaterial.diffuseColour;
    if (gMaterial.hasDiffuseTex != 0)
    {
        float4 texSampleD = diffuseTex.Sample(linearWrapSample, IN.texCoord);
        
        if (texSampleD.a < 0.5f) discard;
        
        albedo *= texSampleD.rgb;
    }
    if (dissolveCB.hasDissolveComponent == 1 && dissolveCB.dissolveData.dissolveAmount > 0)
    {
        float4 dissolveNoisSample = dissolveNoise.Sample(linearWrapSample, IN.texCoord);
        if (dissolveNoisSample.r <= dissolveCB.dissolveData.dissolveAmount)
        {
            discard;
        }
        else if (dissolveNoisSample.r > dissolveCB.dissolveData.dissolveAmount && dissolveNoisSample.r < dissolveCB.dissolveData.dissolveAmount + (dissolveCB.dissolveData.dissolveThickness / 10))
        {
            albedo = dissolveCB.dissolveData.dissolveColor;
            dissolving = true;
        }
    }
    OUT.diffuse = float4(albedo, 1.0f);
    
    
    
    //Load metalic roughness AO texture
    if (gMaterial.hasMetallicRoughnessTex != 0)
    {
        float4 texSampleM = metallicRoughnessTex.Sample(linearWrapSample, IN.texCoord);
    
        metallic = saturate((texSampleM.b) * gMaterial.metallicFactor);
        ao = texSampleM.r;
        alphaRoughness = 1 - clamp(texSampleM.g, minRoughness, 1.0);
    }
    OUT.metalRoughness = float4(ao, alphaRoughness, metallic, 0);
    
    
    //Load normal texture
    if (gMaterial.hasNormalTex != 0)
    {
        float3 tangentNormal = normalTex.Sample(linearWrapSample, IN.texCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
        
        float3 tangentVector = normalize(IN.tangent.xyz);
        float3 bitangentVector = cross(finalWorldNormal, tangentVector);
        float3x3 TBN = float3x3(tangentVector, bitangentVector, finalWorldNormal);
    
        finalWorldNormal = mul(tangentNormal, TBN);
    }
    OUT.normal       = float4(finalWorldNormal, 0.0f);
    
    
    
    //Load emissive texture
    if (gMaterial.hasEmissiveTex != 0)
    {
        float3 emissiveSample = emissiveTex.Sample(linearWrapSample, IN.texCoord);
        
        emissive = emissiveSample.rgb * gMaterial.emissiveColor;
    }
    if (damageHighlightCB.hasDamageHighlightComponent == 1)
    {
        emissive += CalculateDamageHighlight(finalWorldNormal, normalize(gViewPos - IN.worldPos), albedo);
    }
    if (dissolving)
    {
        emissive = dissolveCB.dissolveData.dissolveColor;
    }
    OUT.emissive = float4(emissive, 0.0f);
  
    
    
    // Position
    OUT.position     = float4(IN.worldPos, 0.0f);

    
    return OUT;
}