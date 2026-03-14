#include "NewCBuffers.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);

SamplerState liearSample : register(s0);

static const float PI = 3.14159265f;
static const float EPS = 1e-5f;
static const float3 DIELECTRIC_FRESNEL = 0.04;

float3 SchlickFresnel(float3 F0, float cosTheta)
{
    float x = 1.0f - cosTheta;
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return F0 + (1.0f - F0) * x5;
}

float3 SchlickFresnelMicrofacets(float3 F0, float LdotH)
{
    float x = 1.0f - LdotH;
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return F0 + (1.0f - F0) * x5;
}

float SmithVisibilityFunction(float NdotL, float NdotV, float roughness)
{
    float x1 = NdotV * (1 - roughness) + roughness;
    float x2 = NdotL * x1;

    float y1 = NdotL * (1 - roughness) + roughness;
    float y2 = NdotV * y1;

    float z = x2 + y2;

    return 0.5 / z;
}

float NormalDistributionFunction(float NdotH, float roughness)
{
    float x = NdotH * NdotH;
    float y = roughness * roughness;

    float z = x * y + 1;
    float z2 = z * z;

    float w = PI * z2;

    return y / w;
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

float3 PhongSpecularBRDF(float3 F0, float NdotL, float VdotR, float shininess)
{
    float3 fresnel = SchlickFresnel(F0, NdotL);
    float normalization = (shininess + 2.0f) / (2.0f * PI);
    return normalization * fresnel * pow(VdotR, shininess);
}

float3 MetalicPBR(float3 F0, float LdotH, float NdotL, float NdotV, float NdotH, float roughness)
{
    float3 fresnel = SchlickFresnelMicrofacets(F0, LdotH);
    float visibility = SmithVisibilityFunction(NdotL, NdotV, roughness);
    float distribution = NormalDistributionFunction(NdotH, roughness);

    float normalization = 0.25;

    return normalization * fresnel * visibility * distribution;
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

float3 EvaluateLight(float3 lightDirection, float3 lightColor, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float roughness) //DiffuseBRDF will be base color ("Varies")
{
    float3 halfVector = normalize(viewDirection + lightDirection);
    float lightDotHalf = dot(lightDirection, halfVector);

    float normalDotView = dot(normalVector, viewDirection);

    float noramlDotHalf = dot(normalVector, halfVector);
    
    float normalDotLight = saturate(-dot(lightDirection, normalVector));
    if (normalDotLight <= 0.0f)
        return 0.0f;

    float3 reflectedLight = reflect(lightDirection, normalVector);
    float viewDotReflected = saturate(dot(viewDirection, reflectedLight));
    
    float3 metalicPBR = MetalicPBR(F0, lightDotHalf, normalDotLight, normalDotView, noramlDotHalf, roughness);
    
    return (diffuseBRDF + metalicPBR) * lightColor * normalDotLight;
}

float3 ComputeDirectionalLight(uint lightIndex, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float roughness)
{
    float3 lightDirection = normalize(directionalLights[lightIndex].direction);
    float3 lightColor = directionalLights[lightIndex].color * directionalLights[lightIndex].intensity;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, roughness);
}

float3 ComputePointLight(uint lightIndex, float3 worldPos, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float roughness)
{
    float3 toSurface = worldPos - pointLights[lightIndex].position;
    float distanceToSurface = length(toSurface);
    if (distanceToSurface <= EPS)
        return 0.0f;

    float3 lightDirection = toSurface / distanceToSurface;

    float attenuation = EpicAttenuation(distanceToSurface, pointLights[lightIndex].radius);
    float3 lightColor = pointLights[lightIndex].color * pointLights[lightIndex].intensity * attenuation;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, roughness);
}

float3 ComputeSpotLight(uint lightIndex, float3 worldPos, float3 normalVector, float3 viewDirection, float3 F0, float3 diffuseBRDF, float roughness)
{
    float3 spotDirection = normalize(spotLights[lightIndex].direction);

    float3 toSurface = worldPos - spotLights[lightIndex].position;
    float distanceProjected = dot(toSurface, spotDirection);
    if (distanceProjected <= 0.0f)
        return 0.0f;

    float3 lightDirection = normalize(toSurface);

    float attenuation = EpicAttenuation(distanceProjected, spotLights[lightIndex].radius);

    float cosineAngle = dot(lightDirection, spotDirection);
    float coneAttenuation = SpotConeAttenuation(
        cosineAngle,
        spotLights[lightIndex].cosineInnerAngle,
        spotLights[lightIndex].cosineOuterAngle
    );

    float3 lightColor =
        spotLights[lightIndex].color * spotLights[lightIndex].intensity * attenuation * coneAttenuation;

    return EvaluateLight(lightDirection, lightColor, normalVector, viewDirection, F0, diffuseBRDF, roughness);
}

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float4 texSample = baseColorTex.Sample(liearSample, coord);

    if (hasBaseColorTex != 0 && texSample.a < 0.5f) //The 0.5 is temporary while transparency is not added
    {
        discard;
    }

    float3 albedo = (hasBaseColorTex != 0) ? texSample.rgb * baseColor : baseColor; //Albedo = BaseColor ("Varies")
    
    float2 metallicRoughnessSample = metallicRoughnessTex.Sample(liearSample, coord).rg;
    float2 metallicRoughness = (hasMetallicRoughnessTex != 0)
                                ? metallicRoughnessSample
                                : float2(0, 0.5); 

    float3 normalVector = normalize(normal);
    float3 viewDirection = normalize(viewPos - worldPos);

    float3 F0 = albedo; //Metals

    //Remove this since energy conservation formula is not needed in PBR Metallic Roughness
    //float maxF0 = max(F0.r, max(F0.g, F0.b));
    //float3 albedoEnergy = albedo * (1.0f - maxF0); 
    //float3 diffuseBRDF = albedoEnergy / PI;

    float3 directLightingMetallic = 0.0f;
    float3 directLightingNonMetallic = 0.0f;
    
    float roughness = pow(roughnessFactor, 2);

    // Directional lights
    for (uint i = 0; i < directionalCount; ++i)
    {
        directLightingMetallic += ComputeDirectionalLight(i, normalVector, viewDirection, F0, diffuseBRDF, roughness);
        directLightingNonMetallic += ComputeDirectionalLight(i, normalVector, viewDirection, F0, diffuseBRDF, roughness);

    }

    // Point lights
    for (uint i = 0; i < pointCount; ++i)
    {
        directLighting += ComputePointLight(i, worldPos, normalVector, viewDirection, F0, diffuseBRDF, roughness);
    }

    // Spot lights
    for (uint i = 0; i < spotCount; ++i)
    {
        directLighting += ComputeSpotLight(i, worldPos, normalVector, viewDirection, F0, diffuseBRDF, roughness);
    }

    // Ambient
    float3 indirectLighting = ambientColor * ambientIntensity * albedoEnergy;
    
    float3 colorMapped = PBRNeutralToneMapping(directLighting + indirectLighting);

    return float4(colorMapped, 1.0f);
}