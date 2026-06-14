static const float PI = 3.14159265f;

static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / GAMMA;



float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1 - F0) * pow(1.0 - NdotH, 5);
}

float SmithVisibilityFunction(float NdotL, float NdotV, float roughness)
{
    float NdotVSqr = NdotV * NdotV;
    float NdotLSqr = NdotL * NdotL;
    
    float smithL = NdotL * (NdotV * (1 - roughness) + roughness);
    float smithV = NdotV * (NdotL * (1 - roughness) + roughness);
    
    return 0.5 / (smithL + smithV);
}

float NormalDistributionFunction(float NdotH, float roughness)
{
    float roughnessSqr = roughness * roughness;
    float NdotHSqr = NdotH * NdotH;
    float f = NdotHSqr * (roughnessSqr - 1) + 1;
    
    return roughnessSqr / (PI * f * f);
}

float3 LinearToSRGB(float3 color)
{
    return pow(color, INV_GAMMA);
}