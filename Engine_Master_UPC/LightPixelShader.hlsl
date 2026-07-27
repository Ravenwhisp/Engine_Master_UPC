#include "LightingCBuffers.hlsli"
#include "General.hlsli"
#include "PBRGeneral.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D positionTex : register(t3);
Texture2D emissiveTex : register(t4);

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);
Texture2D shadowMap : register(t11);
Texture2D ssaoTexture : register(t12);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);



//----------DIRECT LIGHTING----------//
float3 LightCalculation(float3 lightDirection, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 diffuseColor, float3 lightColor, float3 F0)
{
    float3 halfVector = normalize(lightDirection + viewDirection);
    
    float NdotL = clamp(-dot(normalVector, lightDirection), 0.001, 1.0);
    float NdotH = saturate(dot(normalVector, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));
    
    float3 fresnel = SchlickFresnel(F0, NdotH);
    float smithVisibility = SmithVisibilityFunction(NdotL, NdotV, alphaRoughness);
    float normalDistribution = NormalDistributionFunction(NdotH, alphaRoughness);
    
    return (diffuseColor + (0.25 * fresnel * smithVisibility * normalDistribution)) * lightColor * NdotL;
}

float3 ComputeDirectionalLight(uint lightIndex, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 lightDirection = normalize(directionalLights[lightIndex].direction);
    float3 lightColor = directionalLights[lightIndex].color * directionalLights[lightIndex].intensity;
    
    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
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

float3 ComputePointLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 toSurface = worldPos - pointLights[lightIndex].position;
    
    float distanceToSurface = length(toSurface);
    if (distanceToSurface <= EPS)
        return 0.0f;

    float attenuation = EpicAttenuation(distanceToSurface, pointLights[lightIndex].radius);
    
    float3 lightDirection = toSurface / distanceToSurface;
    float3 lightColor = pointLights[lightIndex].color * pointLights[lightIndex].intensity * attenuation;
    
    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
}

float SpotConeAttenuation(float cosineAngle, float cosineInner, float cosineOuter)
{
    float denominator = max(cosineInner - cosineOuter, EPS);
    return saturate((cosineAngle - cosineOuter) / denominator);
}

float3 ComputeSpotLight(uint lightIndex, float3 worldPos, float3 viewDirection, float3 normalVector, float NdotV, float alphaRoughness, float3 F0, float3 diffuseColor)
{
    float3 spotDirection = normalize(spotLights[lightIndex].direction);
    float3 toSurface = worldPos - spotLights[lightIndex].position;
    
    float distanceProjected = dot(toSurface, spotDirection);
    if (distanceProjected <= 0.0f)
        return 0.0f;

    float3 lightDirection = normalize(toSurface);
    
    float attenuation = EpicAttenuation(distanceProjected, spotLights[lightIndex].radius);
    float cosineAngle = dot(lightDirection, spotDirection);
    float coneAttenuation = SpotConeAttenuation(cosineAngle, spotLights[lightIndex].cosineInnerAngle, spotLights[lightIndex].cosineOuterAngle);

    float3 lightColor = spotLights[lightIndex].color * spotLights[lightIndex].intensity * attenuation * coneAttenuation;
    
    return LightCalculation(lightDirection, viewDirection, normalVector, NdotV, alphaRoughness, diffuseColor, lightColor, F0);
}
//--------------------//



//----------INDIRECT LIGHTING----------//
float computeSpecularAO(float NdotV, float diffuseAO, float roughness)
{
    return saturate(pow(NdotV + diffuseAO, exp2(-16.0 * roughness - 1.0)) - 1.0 + diffuseAO);
}

float3 getDiffuseAmbientLight(in float3 normal, in float3 baseColour)
{
    float3 irradiance = irradianceTexture.SampleLevel(linearWrapSample, normal, 0).rgb;

    return baseColour * irradiance;
}

void getSpecularAmbientLightNoFresnel(in float3 R, float NdotV, float roughness, in uint numLevels, out float3 firstTerm, out float3 secondTerm)
{
    float3 radiance = environmentTexture.SampleLevel(linearWrapSample, R, roughness * (numLevels - 1)).rgb;
    
    float2 fab = brdfTexture.Sample(linearClampSample, float2(NdotV, roughness)).rg;
    
    firstTerm = radiance * fab.x;
    secondTerm = radiance * fab.y;
}

float3 computeIndirectLighting(in float3 R, in float NdotV, in float3 N, in float3 baseColour, in float roughness, in float roughnessLevels, in float metallic, in float ao, in float specularAO)
{
    float3 diffuse = getDiffuseAmbientLight(N, baseColour);
    diffuse *= ao;

    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, firstTerm, secondTerm);

    float3 metalSpecular = baseColour * firstTerm + secondTerm;
    metalSpecular *= specularAO;

    float3 dielectricSpecular = DIELECTRIC_FRESNEL * firstTerm + secondTerm;
    dielectricSpecular *= specularAO;

    return lerp(diffuse + dielectricSpecular, metalSpecular, metallic);
}
//--------------------//



//----------SHADOW MAPPING----------//
float EvaluateShadowSample(float2 shadowUV, float currentDepth)
{
    float closestDepth = shadowMap.Sample(linearClampSample, shadowUV).r;

    return currentDepth - shadowBias > closestDepth
        ? 1.0f - shadowStrength
        : 1.0f;
}

float ComputeShadow(float3 worldPos)
{
    if (shadowsEnabled == 0)
    {
        return 1.0f;
    }

    float4 shadowPos = mul(float4(worldPos, 1.0f), lightViewProjection);

    if (shadowPos.w == 0.0f)
    {
        return 1.0f;
    }

    shadowPos.xyz /= shadowPos.w;

    float2 shadowUV;
    shadowUV.x = shadowPos.x * 0.5f + 0.5f;
    shadowUV.y = -shadowPos.y * 0.5f + 0.5f;

    float currentDepth = shadowPos.z;

    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f ||
        currentDepth < 0.0f || currentDepth > 1.0f)
    {
        return 1.0f;
    }

    if (pcfEnabled == 0 || pcfRadius == 0)
    {
        return EvaluateShadowSample(shadowUV, currentDepth);
    }

    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    int radius = int(pcfRadius);

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            float2 offset = float2(x, y) * shadowMapTexelSize;
            float2 sampleUV = shadowUV + offset;

            if (sampleUV.x < 0.0f || sampleUV.x > 1.0f ||
                sampleUV.y < 0.0f || sampleUV.y > 1.0f)
            {
                shadowSum += 1.0f;
            }
            else
            {
                shadowSum += EvaluateShadowSample(sampleUV, currentDepth);
            }

            sampleCount += 1.0f;
        }
    }

    return shadowSum / sampleCount;
}
//--------------------//

float SampleSSAO(float4 screenPosition)
{
    if (renderFlags.x < 0.5f)
    {
        return 1.0f;
    }

    float2 ssaoUV = screenPosition.xy * invScreenSize;
    return ssaoTexture.Sample(pointClampSample, ssaoUV).r;
}

float4 main(float4 position : SV_Position, float2 coord : TEXCOORD0) : SV_TARGET
{
    //Initialize material values
    float3 worldPos = positionTex.Sample(linearWrapSample, coord);
    
    
    //Read base color
    float3 albedo = baseColorTex.Sample(linearWrapSample, coord);
    
    
    //Read metalic roughness AO
    float3 metallicRoughnessAOSample = metallicRoughnessTex.Sample(linearWrapSample, coord).rgb;
    float metallic = metallicRoughnessAOSample.b;
    float alphaRoughness = metallicRoughnessAOSample.g;
    float ao = metallicRoughnessAOSample.r;
    
    
    
    //Read emissive
    float3 emissive = emissiveTex.Sample(linearWrapSample, coord);
    //float3 emissive = 0;
    
    
    
    //Read normal
    float3 finalWorldNormal = normalTex.Sample(linearWrapSample, coord).rgb;
    

    
    //Prepare data for render equation
    float3 F0Metallic = albedo;
    float3 F0NonMetallic = 0.04;
    
    float3 diffuseColorMetallic = 0;
    float3 diffuseColorNonMetallic = albedo / PI;
    
    float3 viewDirection = normalize(viewPos - worldPos);
    float3 reflection = normalize(reflect(-viewDirection, finalWorldNormal));
    float NdotV = abs(dot(finalWorldNormal, viewDirection)) + 0.001;
    float horizon = min(1.0 + dot(reflection, finalWorldNormal), 1.0);
    
    alphaRoughness = alphaRoughness * alphaRoughness;
    
    
    
    //Calculate directional direct lighting
    float3 directionalMetallic = 0.0f;
    float3 directionalNonMetallic = 0.0f;

    for (uint i = 0; i < directionalCount; ++i)
    {
        directionalMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        directionalNonMetallic += ComputeDirectionalLight(i, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    
    
    //Calculate point and spot direct lighting
    float3 otherMetallic = 0.0f;
    float3 otherNonMetallic = 0.0f;

    for (uint i = 0; i < pointCount; ++i)
    {
        otherMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputePointLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    for (uint i = 0; i < spotCount; ++i)
    {
        otherMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0Metallic, diffuseColorMetallic);
        otherNonMetallic += ComputeSpotLight(i, worldPos, viewDirection, finalWorldNormal, NdotV, alphaRoughness, F0NonMetallic, diffuseColorNonMetallic);
    }

    
    
    //Apply shadow only to directional direct lighting
    float shadow = ComputeShadow(worldPos);

    float3 directionalLighting = lerp(directionalNonMetallic, directionalMetallic, metallic);
    float3 otherLighting = lerp(otherNonMetallic, otherMetallic, metallic);

    float3 directLighting = directionalLighting * shadow + otherLighting;


    
    //Calculate indirect lighting
    float ssao = SampleSSAO(position);

    if (renderFlags.y > 0.5f)
    {
        return float4(ssao.xxx, 1.0f);
    }

    float diffuseAO = saturate(ao * ssao);

    float specularAO = computeSpecularAO(NdotV, diffuseAO, alphaRoughness);
    specularAO *= horizon;

    float3 indirectLighting = computeIndirectLighting(reflection, NdotV, finalWorldNormal, F0Metallic, alphaRoughness, 11, metallic, diffuseAO, specularAO);
    
    
    //Calculate final color
    // Output linear HDR colour. Exposure, tone mapping and gamma correction are
    // applied later by the post-process pass.
    float3 finalColor = directLighting + indirectLighting + emissive;

    
    
    return float4(finalColor, 1.0f);
}