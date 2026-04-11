TextureCube skyTexture : register(t0);
SamplerState skySampler : register(s0);

static const uint NUM_SAMPLES = 100;
static const float PI = 3.14159265f;


float3x3 computeTangetSpace(in float3 normal)
{
    // Choose appropriate up vector to avoid singularity
    float3 up = abs(normal.y) > 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);

    // Compute tangent (right) vector
    float3 right = normalize(cross(up, normal));

    // Recompute bitangent (up) vector to ensure orthogonality
    up = cross(normal, right);

    // Construct and return TBN matrix
    return float3x3(right, up, normal);
}

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

float2 hammersley2D(uint sampleIndex, uint numSamples)
{
    return float2(float(sampleIndex) / float(numSamples), radicalInverse_VdC(sampleIndex));
}

float3 hemisphereSample(float u1, float u2)
{
    // Convert uniform random to spherical coordinates
    float phi = u1 * 2.0 * PI;
    float radius = sqrt(u2);

    // Convert to Cartesian coordinates (cosine-weighted)
    return float3(radius * cos(phi), radius * sin(phi), sqrt(1.0 - u2));
}

float4 main(float3 direction : TEXCOORD0) : SV_TARGET
{
    float3 irradiance = 0.0;
    float3 normal = normalize(direction);
    float3x3 tangentSpace = computeTangetSpace(normal);
    
    // Monte Carlo integration loop
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        // Generate low-discrepancy sample point
        float2 randomValue = hammersley2D(i, NUM_SAMPLES);

        // Sample hemisphere and transform to world space
        float3 sampleDir = hemisphereSample(randomValue.x, randomValue.y);
        float3 worldDir = mul(sampleDir, tangentSpace);

        // Sample environment map and accumulate radiance
        float3 sampleRadiance = skyTexture.SampleLevel(skySampler, worldDir, 0).rgb;
        irradiance += sampleRadiance;
    }
    
    // Average samples and return final irradiance
    return float4(irradiance * (1.0 / NUM_SAMPLES), 1.0);
}

