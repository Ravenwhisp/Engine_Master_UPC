#include "CBuffers.hlsli"

Texture2D colourTex : register(t0);
SamplerState colourSampler : register(s0);

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD) : SV_TARGET
{
    // Sample diffuse color
    float3 Cd = hasDiffuseTex ? colourTex.Sample(colourSampler, texCoord).rgb * diffuseColour.rgb : diffuseColour.rgb;

    // Normalize vectors
    float3 N = normalize(normal);
    float3 L = normalize(lightDirection);
    float3 V = normalize(view - worldPos);

    float3 R = reflect(L, N);

    // Calculate lighting components
    float diffuse = saturate(-dot(L, N));
    float specular = pow(saturate(dot(V, R)), shininess);

    float Rf_max = max(max(specularColour.r, specularColour.g), specularColour.b);
    float energyConservation = (shininess + 2) / 2 ;
    float fresnel = specularColour + (1 - specularColour) * pow(1 - diffuse, 5);

    // Combine lighting
    float3 brdfPhongColour = ((Cd * (1 - Rf_max)) + energyConservation * fresnel * specular) * lightColor * diffuse + ambientColor * Cd;

    return float4(brdfPhongColour, 1.0f);
}