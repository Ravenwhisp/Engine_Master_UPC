#include "PBRLighting.hlsli"


struct MaterialData
{
    float3 diffuseColour;
    uint hasDiffuseTex;

    float metallicFactor;
    float roughnessFactor;
    uint hasMetallicRoughnessTex;
    float normalFactor;

    uint hasNormalTex;
    float3 emissiveColor;

    uint hasEmissiveTex;
    float3 padding;
};

cbuffer ModelDataCB : register(b4)
{
    float4x4 gModel;
    float4x4 gNormalMat;
    MaterialData gMaterial;
};


struct DamageHighlightData
{
    float damageHighlight;
    float3 damageHighlightCenterColor;

    float3 damageHighlightRimColor;
    float damageHighlightRimIntensity;
};

struct DamageHighlightCB
{
    uint hasDamageHighlightComponent;
    float3 padding1;

    DamageHighlightData damageHighlightData;
};

struct DissolveData
{
    float dissolveAmount;
    float3 dissolveColor;

    float dissolveThickness;
};

struct DissolveCB
{
    uint hasDissolveComponent;
    float3 padding1;

    DissolveData dissolveData;
    float3 padding2;
};

cbuffer VisualEffectsCB : register(b5)
{
    DamageHighlightCB damageHighlightCB;
    DissolveCB dissolveCB;
};

cbuffer RevealSettings : register(b6)
{
    float depthBias;
    float revealAlpha;
};


Texture2D diffuseTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D emissiveTex : register(t3);

Texture2D<float> mainDepth : register(t12);
Texture2D<float> occluderEligibility : register(t13);
Texture2D dissolveNoise : register(t14);


struct PSInput
{
    float4 clipPos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};


float3 CalculateDamageHighlight(float3 normalVector, float3 viewDirection, float3 albedo)
{
    float NdotV = saturate(dot(normalVector, viewDirection));

    float3 fresnelColor = ColoredSchlickFresnel(
        damageHighlightCB.damageHighlightData.damageHighlightCenterColor * albedo,
        damageHighlightCB.damageHighlightData.damageHighlightRimColor,
        NdotV,
        25.0f - damageHighlightCB.damageHighlightData.damageHighlightRimIntensity
    );

    return fresnelColor * damageHighlightCB.damageHighlightData.damageHighlight;
}


float4 main(PSInput input) : SV_Target0
{
    int2 pixel = int2(input.clipPos.xy);

    float sceneDepth = mainDepth.Load(int3(pixel, 0));
    float eligibility = occluderEligibility.Load(int3(pixel, 0));
    float playerDepth = input.clipPos.z;

    if (eligibility <= 0.5f)
        discard;

    if (playerDepth <= sceneDepth + depthBias)
        discard;


    float metallic = gMaterial.metallicFactor;
    float alphaRoughness = gMaterial.roughnessFactor;
    float minRoughness = 0.04f;
    float ao = 1.0f;

    float3 emissive = 0.0f;
    float3 finalWorldNormal = normalize(input.normal);

    bool dissolving = false;


    float3 albedo = gMaterial.diffuseColour;

    if (gMaterial.hasDiffuseTex != 0)
    {
        float4 diffuseSample = diffuseTex.Sample(linearWrapSample, input.texCoord);

        if (diffuseSample.a < 0.5f)
            discard;

        albedo *= diffuseSample.rgb;
    }


    if (dissolveCB.hasDissolveComponent == 1 && dissolveCB.dissolveData.dissolveAmount > 0.0f)
    {
        float dissolveSample = dissolveNoise.Sample(linearWrapSample, input.texCoord).r;

        if (dissolveSample <= dissolveCB.dissolveData.dissolveAmount)
        {
            discard;
        }
        else if (dissolveSample < dissolveCB.dissolveData.dissolveAmount + (dissolveCB.dissolveData.dissolveThickness / 10.0f))
        {
            albedo = dissolveCB.dissolveData.dissolveColor;
            dissolving = true;
        }
    }


    if (gMaterial.hasMetallicRoughnessTex != 0)
    {
        float4 metallicRoughnessSample = metallicRoughnessTex.Sample(linearWrapSample, input.texCoord);

        metallic = saturate(metallicRoughnessSample.b * gMaterial.metallicFactor);
        ao = metallicRoughnessSample.r;
        alphaRoughness = 1.0f - clamp(metallicRoughnessSample.g, minRoughness, 1.0f);
    }


    if (gMaterial.hasNormalTex != 0)
    {
        float3 tangentNormal = normalTex.Sample(linearWrapSample, input.texCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0f - 1.0f);

        float3 tangentVector = normalize(input.tangent);
        float3 bitangentVector = cross(finalWorldNormal, tangentVector);
        float3x3 TBN = float3x3(tangentVector, bitangentVector, finalWorldNormal);

        finalWorldNormal = mul(tangentNormal, TBN);
    }


    if (gMaterial.hasEmissiveTex != 0)
    {
        float3 emissiveSample = emissiveTex.Sample(linearWrapSample, input.texCoord).rgb;
        emissive = emissiveSample * gMaterial.emissiveColor;
    }


    if (damageHighlightCB.hasDamageHighlightComponent == 1)
        emissive += CalculateDamageHighlight(finalWorldNormal, normalize(viewPos - input.worldPos), albedo);

    if (dissolving)
        emissive = dissolveCB.dissolveData.dissolveColor;


    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04f;

    float3 diffuseColorMetallic = 0.0f;
    float3 diffuseColorNonMetallic = albedo / PI;

    float3 viewDirection = normalize(viewPos - input.worldPos);
    float3 reflection = normalize(reflect(-viewDirection, finalWorldNormal));

    float NdotV = abs(dot(finalWorldNormal, viewDirection)) + 0.001f;
    float horizon = min(1.0f + dot(reflection, finalWorldNormal), 1.0f);

    alphaRoughness = alphaRoughness * alphaRoughness;


    float3 directionalMetallic = 0.0f;
    float3 directionalNonMetallic = 0.0f;

    for (uint i = 0; i < directionalCount; ++i)
    {
        directionalMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        directionalNonMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }


    float3 otherMetallic = 0.0f;
    float3 otherNonMetallic = 0.0f;

    for (uint i = 0; i < pointCount; ++i)
    {
        otherMetallic += ComputePointLight(i, input.worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputePointLight(i, input.worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    for (uint i = 0; i < spotCount; ++i)
    {
        otherMetallic += ComputeSpotLight(i, input.worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputeSpotLight(i, input.worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }


    uint selectedCascadeIndex;
    float shadow = ComputeShadow(input.worldPos, selectedCascadeIndex);

    float3 directionalLighting = lerp(directionalNonMetallic, directionalMetallic, metallic);
    float3 otherLighting = lerp(otherNonMetallic, otherMetallic, metallic);

    float3 directLighting = directionalLighting * shadow + otherLighting;


    // Deliberately do not sample screen-space SSAO here:
    // that pixel belongs to the occluding scene surface, not the hidden player.
    float diffuseAO = saturate(ao);

    float specularAO = computeSpecularAO(NdotV, diffuseAO, alphaRoughness);
    specularAO *= horizon;

    float3 indirectLighting = computeIndirectLighting(
        reflection,
        NdotV,
        finalWorldNormal,
        F0Metallic,
        alphaRoughness,
        11,
        metallic,
        diffuseAO,
        specularAO
    );


    float3 finalColor = directLighting + indirectLighting + emissive;

    if (cascadePadding.x > 0.5f && selectedCascadeIndex < MAX_SHADOW_CASCADES)
        finalColor = lerp(finalColor, GetCascadeDebugColor(selectedCascadeIndex), 0.35f);

    return float4(finalColor, revealAlpha);
}