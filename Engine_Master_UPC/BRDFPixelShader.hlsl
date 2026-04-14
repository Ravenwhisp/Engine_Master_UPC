static const float PI = 3.14159265f;
static const uint NUM_SAMPLES = 256;

float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    // Convert to float in [0, 1) range
    return float(bits) * 2.3283064365386963e-10; // Divide by 2^32
}

float V_GGX(float NdotL, float NdotV, float roughness)
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


float2 hammersley2D(uint sampleIndex, uint numSamples)
{
    return float2(float(sampleIndex) / float(numSamples), radicalInverse_VdC(sampleIndex));
}

// GGX NDF Importance Sampling for Specular IBL
float3 ggxSample(float2 randomU, float alphaRoughness)
{
    // Generate uniform distribution in azimuth
    float azimuth = 2.0 * PI * randomU.x;

    // Sample elevation angle based on GGX distribution
    float a2 = alphaRoughness * alphaRoughness;
    float cosElevation = sqrt((1.0 - randomU.y) / (randomU.y * (a2 - 1.0) + 1.0));
    float sinElevation = sqrt(1.0 - cosElevation * cosElevation);

    // Convert to Cartesian direction vector
    return float3(cos(azimuth) * sinElevation, sin(azimuth) * sinElevation, cosElevation);
}

float4 EnvironmentBRDFPS(float2 uv : TEXCOORD) : SV_Target
{
    float NdotV = uv.x;
    float alphaRoughness = uv.y * uv.y;

    float3 V = (sqrt(1.0 - NdotV * NdotV), 0.0, NdotV); // Construct view vector from NdotV
    float3 N = float3(0.0, 0.0, 1.0); // Local normal vector points along Z-axis
    precise float A = 0.0;
    precise float B = 0.0;

    for (uint i = 0; i < NUM_SAMPLES; i++)
    {
        float2 randomSeq = hammersley2D(i, NUM_SAMPLES);
        float3 H = ggxSample(randomSeq, alphaRoughness);
        float3 L = reflect(-V, H);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            // Calculate visibility term and PDF
            float V_pdf = V_GGX(NdotL, NdotV, alphaRoughness) * 4 * VdotH * NdotL / NdotH;
             // Fresnel term approximation
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * V_pdf;
            B += Fc * V_pdf;
        }
    }

    return float4(A / float(NUM_SAMPLES), B / float(NUM_SAMPLES), 0.0, 1.0);
}