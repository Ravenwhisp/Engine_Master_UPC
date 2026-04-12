TextureCube skyTexture : register(t0);
SamplerState skySampler : register(s0);
cbuffer EnvironmentData : register(b1)
{
    float roughness;
    float3 environmentPadding;
};

static const uint NUM_SAMPLES = 256;
static const float PI = 3.14159265f;

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

// GGX NDF Importance Sampling for Specular IBL
float3 hemisphereSampleGGX(float2 randomU, float alphaRoughness)
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

float4 main(float3 texcoords : TEXCOORD) : SV_TARGET
{
    float3 R = normalize(texcoords); // Normalize input direction
    float3 N = R, V = R;
    float3 color = 0.0;
    float weight = 0.0;
    float3x3 tangentSpace = computeTangetSpace(N); // Compute tangent space for sampling
    float alphaRoughness = max(roughness * roughness, 0.001); // Calculate and clamp roughness to avoid singularities
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 randomSeq = hammersley2D(i, NUM_SAMPLES);
        float3 dir = hemisphereSampleGGX(randomSeq, alphaRoughness); // Generate sample direction using GGX distribution
        float3 H = normalize(mul(dir, tangentSpace)); // Transform to world space and calculate lighting direction
        float3 L = reflect(-V, H);
        float NdotL = dot(N, L); // Sample environment if facing positive direction
        if (NdotL > 0)
        {
            color += skyTexture.Sample(skySampler, L).rgb * NdotL;
            weight += NdotL;
        }
    }
    // Return normalized result
    return float4(color / weight, 1.0);
}