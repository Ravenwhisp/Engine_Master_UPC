#include "NewCBuffers.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);

static const float PI = 3.14159265f;
static const float EPS = 1e-5f;
static const float3 DIELECTRIC_FRESNEL = 0.04;

static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / GAMMA;



float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1 - F0) * pow(1.0 - NdotH, 5);
}

float SmithVisibilityFunction(float NdotL, float NdotV, float roughness)
{
    float roughnessSqr = roughness * roughness;
    float NdotVSqr = NdotV * NdotV;
    float NdotLSqr = NdotL * NdotL;
    
    float smithL = NdotL * (NdotV * (1 - roughness) + roughness);
    float smithV = NdotV * (NdotL * (1 - roughness) + roughness);
    
    //float smithV = (2 * NdotV) / (NdotV + sqrt(roughnessSqr + (1 - roughnessSqr) * NdotVSqr));
    //float smithL = (2 * NdotL) / (NdotL + sqrt(roughnessSqr + (1 - roughnessSqr) * NdotLSqr));
    
    return 0.5 / (smithL + smithV);
}

float NormalDistributionFunction(float NdotH, float roughness)
{
    float roughnessSqr = roughness * roughness;
    float NdotHSqr = NdotH * NdotH;
    float f = NdotHSqr * (roughnessSqr - 1) + 1;
    
    return roughnessSqr / (PI * f * f);
}

float EpicAttenuation(float distanceValue, float radiusValue)
{
    if (radiusValue <= EPS)
        return 0.0f;

    float normalizedDistance = distanceValue / radiusValue;
    float normalizedDistance2 = normalizedDistance * normalizedDistance;
    float normalizedDistance4 = normalizedDistance2 * normalizedDistance2;

    float numerator = max(1.0f - normalizedDistance4, 0.0f);
    numerator *= numerator;

    float denominator = distanceValue * distanceValue + 1.0f;
    return numerator / denominator;
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, EPS);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 PBRNeutralToneMapping(float3 color)
{
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    float peak = max(color.r, max(color.g, color.b));
    const float startCompression = 0.8 - 0.04;

    if (peak < startCompression)
    {
        return color;
    }
    const float d = 1. - startCompression;
    float newPeak = 1. - d * d / (peak + d - startCompression);
    color *= newPeak / peak;
    
    const float desaturation = 0.15;
    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return lerp(color, newPeak.xxx, g);
}

float3 ComputeDirectionalLight(uint lightIndex, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 lightDirection = normalize(directionalLights[lightIndex].direction);
    lightDirection *= -1;
    float3 lightColor = directionalLights[lightIndex].color * directionalLights[lightIndex].intensity;
    
    //------Move into function and return------//
    float3 halfVector = normalize(lightDirection + viewDirection);
    
    float NdotL = clamp(dot(normalVector, lightDirection), 0.001, 1.0);
    float NdotH = saturate(dot(normalVector, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));
    
    float3 fresnel = SchlickFresnel(F0, NdotH);
    float smithVisibility = SmithVisibilityFunction(NdotL, NdotV, alphaRoughness);
    float normalDistribution = NormalDistributionFunction(NdotH, alphaRoughness);
    
    return (diffuseColor + (0.25 * fresnel * smithVisibility * normalDistribution)) * lightColor * NdotL;
    //-----------------------------------------//
}

float3 ComputePointLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 toSurface = worldPos - pointLights[lightIndex].position;
    float distanceToSurface = length(toSurface);
    
    if (distanceToSurface <= EPS) return 0.0f;

    float3 lightDirection = toSurface / distanceToSurface;
    lightDirection *= -1;
    
    float attenuation = EpicAttenuation(distanceToSurface, pointLights[lightIndex].radius);
    float3 lightColor = pointLights[lightIndex].color * pointLights[lightIndex].intensity * attenuation;

    //------Move into function and return------//
    float3 halfVector = normalize(lightDirection + viewDirection);
    
    float NdotL = clamp(dot(normalVector, lightDirection), 0.001, 1.0);
    float NdotH = saturate(dot(normalVector, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));
    
    float3 fresnel = SchlickFresnel(F0, NdotH);
    float smithVisibility = SmithVisibilityFunction(NdotL, NdotV, alphaRoughness);
    float normalDistribution = NormalDistributionFunction(NdotH, alphaRoughness);
    
    return (diffuseColor + (0.25 * fresnel * smithVisibility * normalDistribution)) * lightColor * NdotL;
    //-----------------------------------------//
}

float3 ComputeSpotLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 spotDirection = normalize(spotLights[lightIndex].direction);

    float3 toSurface = worldPos - spotLights[lightIndex].position;
    float distanceProjected = dot(toSurface, spotDirection);
    
    if (distanceProjected <= 0.0f) return 0.0f;

    float3 lightDirection = normalize(toSurface);
    lightDirection *= -1;
    
    float attenuation = EpicAttenuation(distanceProjected, spotLights[lightIndex].radius);
    float cosineAngle = dot(lightDirection, spotDirection);
    float coneAttenuation = SpotConeAttenuation( cosineAngle, spotLights[lightIndex].cosineInnerAngle, spotLights[lightIndex].cosineOuterAngle );

    float3 lightColor = spotLights[lightIndex].color * spotLights[lightIndex].intensity * attenuation * coneAttenuation;

    //------Move into function and return------//
    float3 halfVector = normalize(lightDirection + viewDirection);
    
    float NdotL = clamp(dot(normalVector, lightDirection), 0.001, 1.0);
    float NdotH = saturate(dot(normalVector, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));
    
    float3 fresnel = SchlickFresnel(F0, NdotH);
    float smithVisibility = SmithVisibilityFunction(NdotL, NdotV, alphaRoughness);
    float normalDistribution = NormalDistributionFunction(NdotH, alphaRoughness);
    
    return (diffuseColor + (0.25 * fresnel * smithVisibility * normalDistribution)) * lightColor * NdotL;
    //-----------------------------------------//
}

float3 LinearToSRGB(float3 color)
{
    return pow(color, INV_GAMMA);
}

float3 getDiffuseAmbientLight(in float3 normal, in float3 baseColour)
{
    float3 irradiance = irradianceTexture.SampleLevel(linearWrapSample, normal, 0).rgb;

    return baseColour * irradiance;
}

float3 getSpecularAmbientLight(in float3 R, float NdotV, float roughness, in uint numLevels, float3 F0)
{

    float3 radiance = environmentTexture.SampleLevel(linearWrapSample, R, roughness * (numLevels - 1)).rgb;

    float2 fab = brdfTexture.Sample(linearWrapSample, float2(NdotV, roughness)).rg;

    return radiance * (F0 * fab.x + fab.y);
}

float computeLod(float pdf, int numSamples, int width)
{
    float solidAngle = 1.0 / ((float) numSamples * pdf + 1e-6);

    float texelSolidAngle = 1.0 / (6.0 * width * width);

    return max(0.5 * log2(solidAngle / texelSolidAngle), 0.0);
}

void getSpecularAmbientLightNoFresnel(in float3 R, float NdotV, float roughness, in uint numLevels, out float3 firstTerm, out float3 secondTerm) {

    float3 radiance = environmentTexture.SampleLevel(linearWrapSample, R, roughness * (numLevels - 1)).rgb;
    
    float2 fab = brdfTexture.Sample(linearClampSample, float2(NdotV, roughness)).rg;
    
     firstTerm = radiance * fab.x;
     secondTerm = radiance * fab.y;
}

float3 computeLighting(in float3 V, in float3 N, in float3 baseColour, in float roughness, in float roughnessLevels, in float metallic )
{
    float3 R = reflect(-V, N);
    float NdotV = saturate(dot(N, V));
    float3 diffuse = getDiffuseAmbientLight(N, baseColour);
    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, firstTerm, secondTerm);

    float3 metalSpecular = baseColour * firstTerm + secondTerm;

    float3 dielectricSpecular = 0.04 * firstTerm + secondTerm;
    return lerp(diffuse + dielectricSpecular, metalSpecular, metallic);
}

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float3 tangent : TANGENT, float2 coord : TEXCOORD) : SV_TARGET 
{
    //Load texture & material data
    float4 texSample = baseColorTex.Sample(linearWrapSample, coord);
    if (hasBaseColorTex != 0 && texSample.a < 0.5f)
    {
        discard;
    }
    float3 albedo = (hasBaseColorTex != 0) ? texSample.rgb * baseColor : baseColor;
    
    float2 metallicRoughnessSample = metallicRoughnessTex.Sample(linearWrapSample, coord).bg;
    float metallic = hasMetallicRoughnessTex != 0 ? 1 - saturate(metallicRoughnessSample.x * metallicFactor) : metallicFactor;
    metallic = 0;
    
    float minRoughness = 0.04;
    //float perceptualRoughness = hasMetallicRoughnessTex != 0 ? clamp((1 - metallicRoughnessSample.y) * roughnessFactor, minRoughness, 1.0) : roughnessFactor;
    float perceptualRoughness = hasMetallicRoughnessTex != 0 ? clamp(metallicRoughnessSample.y * 1, minRoughness, 1.0) : roughnessFactor;
    //float alphaRoughness = perceptualRoughness * perceptualRoughness;
    float alphaRoughness = perceptualRoughness;
    
    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04;
    
    float3 diffuseColorMetallic = 0;
    float3 diffuseColorNonMetallic = albedo / PI;
    
    float3 normalVector = normalize(normal);
    float3 tangentVector = normalize(tangent.xyz);
    float3 bitangentVector = cross(normalVector, tangentVector);
    float3x3 TBN = float3x3(tangentVector, bitangentVector, normalVector);
    
    float3 tangentNormal = normalTex.Sample(linearWrapSample, coord).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    
    float3 finalWorldNormal = mul(tangentNormal, TBN);
    
    float3 viewDirection = normalize(viewPos - worldPos);
    float3 reflection = -normalize(reflect(viewDirection, normal));
    
    float NdotV = abs(dot(finalWorldNormal, viewDirection)) + 0.001;
    
    float3 colorMetallic = 0.0;
    float3 colorNonMetallic = 0.0;

    // Directional lights
    for (uint i = 0; i < directionalCount; ++i)
    {
        colorMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    // Point lights
    for (uint i = 0; i < pointCount; ++i)
    {
        colorMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    // Spot lights
    for (uint i = 0; i < spotCount; ++i)
    {
        colorMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }
        
    // Ambient
    float3 directLighting = lerp(colorNonMetallic, colorMetallic, metallic);
    
    //IBL
    float3 indirectLighting = computeLighting(viewDirection, finalWorldNormal, F0Metallic, alphaRoughness, 11, metallic);
    
    float3 colorMapped = PBRNeutralToneMapping(directLighting + indirectLighting);
    float3 finalColor = LinearToSRGB(colorMapped);

    return float4(finalColor, 1.0f);
}