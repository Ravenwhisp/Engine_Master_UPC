#include "LightingCBuffers.hlsli"

// These correspond to the output of GBufferPS.hlsl
Texture2D gBufferDiffuseTex : register(t0);                 // albedo
Texture2D gBufferMetallicRoughnessTex : register(t1);       // metallic = R + roughness = G
Texture2D gBufferNormalTex : register(t2);                  // world normal
Texture2D gBufferPositionTex : register(t3);                // world position

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);

SamplerState linearSample : register(s0);

static const float PI = 3.14159265f;
static const float EPS = 1e-5f;
static const float3 DIELECTRIC_FRESNEL = 0.04;

static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / GAMMA;

//float3 SchlickFresnel(float3 F0, float cosTheta)
//{
    
    
//    float x = 1.0f - cosTheta;
//    float x2 = x * x;
//    float x5 = x2 * x2 * x;
//    return F0 + (1.0f - F0) * x5;
//}

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
    //float ndotHSqr = NdotH * NdotH;
    //float tanNdotHSqr = (1 - ndotHSqr) / ndotHSqr;
    
    return roughnessSqr / (PI * f * f);
    //return (1.0 / PI) * pow(roughness / (ndotHSqr * (roughnessSqr + tanNdotHSqr)), 2);
    
    
    //float z = x * y + 1;
    //float z2 = z * z;

    //float w = PI * z2;

    //return y / w;
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

//float3 PhongSpecularBRDF(float3 F0, float NdotL, float VdotR, float shininess)
//{
//    float3 fresnel = SchlickFresnel(F0, NdotL);
//    float normalization = (shininess + 2.0f) / (2.0f * PI);
//    return normalization * fresnel * pow(VdotR, shininess);
//}

//float3 MetalicPBR(float3 F0, float LdotH, float NdotL, float NdotV, float NdotH, float roughness)
//{
    
    
    
    
    

//    float normalization = 0.25;

//    return (specularDistribution * fresnel * geometricShadow) / (4 * (NdotL * NdotV));
//}

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

//float3 EvaluateLight(float3 lightDirection, float3 lightColor, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float roughness) //DiffuseBRDF will be base color ("Varies")
//{
//    float3 halfVector = normalize(viewDirection + lightDirection);
//    float lightDotHalf = dot(lightDirection, halfVector);

//    float normalDotView = dot(normalVector, viewDirection);

//    float noramlDotHalf = dot(normalVector, halfVector);
    
//    float normalDotLight = saturate(-dot(lightDirection, normalVector));
//    if (normalDotLight <= 0.0f)
//        return 0.0f;

//    float3 reflectedLight = reflect(lightDirection, normalVector);
//    float viewDotReflected = saturate(dot(viewDirection, reflectedLight));
    
//    float3 metalicPBR = MetalicPBR(F0, lightDotHalf, normalDotLight, normalDotView, noramlDotHalf, roughness);
    
//    float3 lightModel = (diffuseBRDF + metalicPBR);
//    lightModel *= normalDotLight;
//    return lightModel * lightColor;
//}

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
    
    //return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, alphaRoughness);
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
    float3 irradiance = irradianceTexture.SampleLevel(linearSample, normal, 0).rgb;

    return baseColour * irradiance;
}

float3 getSpecularAmbientLight(in float3 R, float NdotV, float roughness, in uint numLevels, float3 F0)
{

    // Sample prefiltered environment map at appropriate mip level
    float3 radiance = environmentTexture.SampleLevel(linearSample, R, roughness * (numLevels - 1)).rgb;

    // Look up BRDF scale and bias terms from LUT
    float2 fab = brdfTexture.Sample(linearSample, float2(NdotV, roughness)).rg;

    // Combine F0 with BRDF terms and modulate by environment radiance
    return radiance * (F0 * fab.x + fab.y);
}

// Compute the appropriate mip level based on solid angle probability
float computeLod(float pdf, int numSamples, int width)
{
    // Probability of each sample - larger for less likely samples
    float solidAngle = 1.0 / ((float) numSamples * pdf + 1e-6);

     // Probability of each texel - smaller for larger cubemaps
    float texelSolidAngle = 1.0 / (6.0 * width * width);

    // Mip level calculation: 0.5*log2 = log4 since each mip level is 4x smaller
    return max(0.5 * log2(solidAngle / texelSolidAngle), 0.0);
}

void getSpecularAmbientLightNoFresnel(in float3 R, float NdotV, float roughness, in uint numLevels, out float3 firstTerm, out float3 secondTerm) {

    // Sample prefiltered environment map at appropriate mip level
    float3 radiance = environmentTexture.SampleLevel(linearSample, R, roughness * (numLevels - 1)).rgb;
    
    // Look up BRDF scale and bias terms from LUT
    float2 fab = brdfTexture.Sample(linearSample, float2(NdotV, roughness)).rg;
    
     // F0 fresnel term removed
     firstTerm = radiance * fab.x;
     secondTerm = radiance * fab.y;
}

float3 computeLighting(in float3 V, in float3 N, in float3 baseColour, in float roughness, in float roughnessLevels, in float metallic )
{
    float3 R = reflect(-V, N);
    float NdotV = saturate(dot(N, V));
    // Diffuse lighting for non-metallic surfaces
    // Metals have no diffuse component (all specular)
    float3 diffuse = getDiffuseAmbientLight(N, baseColour);
    // Get two terms of cached radiance
    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, firstTerm, secondTerm);

    // Metallic specular: uses base color as Fresnel Value (and colour)
    float3 metalSpecular = baseColour * firstTerm + secondTerm;

    // Dielectric specular: uses constant Fresnel value (0.04 for non-metals)
    float3 dielectricSpecular = 0.04 * firstTerm + secondTerm;
    // Blend between dielectric (diffuse + specular) and metallic (specular only)
    return lerp(diffuse + dielectricSpecular, metalSpecular, metallic);
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_TARGET
{    
    float3 worldPos = gBufferPositionTex.Sample(linearSample, uv);
    float3 normal = gBufferNormalTex.Sample(linearSample, uv);
    
    float3 albedo = gBufferDiffuseTex.Sample(linearSample, uv).xyz;
    
    float4 metallicRoughnessTexSample = gBufferMetallicRoughnessTex.Sample(linearSample, uv);
    float metallic = metallicRoughnessTexSample.r;
    float perceptualRoughness = metallicRoughnessTexSample.g;
    
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    
    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04;
    
    float3 diffuseColorMetallic = 0;
    float3 diffuseColorNonMetallic = albedo / PI;
    
    //float3 diffuseColor = (albedo * (float3(1.0, 1.0, 1.0) - f0)) * (1.0 - metallic);
    //diffuseColor = diffuseColor / PI;
    //float3 specularColor = lerp(f0, albedo, metallic);
    
    //float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    //float reflectance90 = saturate(reflectance * 25.0);
    //float3 specularEnviromentR0 = specularColor.rgb;
    //float3 specularEnviromentR90 = float3(1.0, 1.0, 1.0) * reflectance90;
    
    float3 normalVector = normalize(normal);
    
    float3 viewDirection = normalize(viewPos - worldPos);
    float3 reflection = -normalize(reflect(viewDirection, normal));
    
    float NdotV = abs(dot(normalVector, viewDirection)) + 0.001;
    
    float3 colorMetallic = 0.0;
    float3 colorNonMetallic = 0.0;

    //viewDirection, normalVector, NdotV, alphaRoughness, specularEnviromentR0, specularEnviromentR90, diffuseColor
    
    // Directional lights
    for (uint i = 0; i < directionalCount; ++i)
    {
        colorMetallic += ComputeDirectionalLight(i, viewDirection, normalVector, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputeDirectionalLight(i, viewDirection, normalVector, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    // Point lights
    for (uint i = 0; i < pointCount; ++i)
    {
        colorMetallic += ComputePointLight(i, worldPos, viewDirection, normalVector, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputePointLight(i, worldPos, viewDirection, normalVector, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    // Spot lights
    for (uint i = 0; i < spotCount; ++i)
    {
        colorMetallic += ComputeSpotLight(i, worldPos, viewDirection, normalVector, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        colorNonMetallic += ComputeSpotLight(i, worldPos, viewDirection, normalVector, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }
        
    // Ambient
    float3 directLighting = lerp(colorNonMetallic, colorMetallic, metallic);
    
    //IBL
    //float3 indirectLighting = ambientColor * ambientIntensity;
    //float3 indirectLighting = getDiffuseAmbientLight(normalVector, baseColor);
    float3 indirectLighting = computeLighting(viewDirection, normalVector, F0Metallic, alphaRoughness, 5, metallic);
    
    float3 colorMapped = PBRNeutralToneMapping(directLighting + indirectLighting);
    //float3 finalColor = LinearToSRGB(colorMapped);
    float3 finalColor = LinearToSRGB(colorMapped);
    
    //return float4(metallicRoughnessTexSample.rgb, 1.0f);
    return float4(finalColor, 1.0f);
}