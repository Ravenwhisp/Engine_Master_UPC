cbuffer ApplyConstants : register(b0)
{
    float nearDistance;
    float maxDistance;
    float projectionA;
    float projectionB;

    uint gridDepth;
    uint padding0;
    uint padding1;
    uint padding2;
};

Texture3D<float4> integratedVolume : register(t0);
Texture2D<float> depthTexture : register(t1);
SamplerState linearClampSampler : register(s0);

float LinearizeViewDepth(float deviceDepth)
{
    float denominator = deviceDepth + projectionA;

    if (abs(denominator) < 0.000001f)
        denominator = denominator < 0.0f ? -0.000001f : 0.000001f;

    float viewZ = -projectionB / denominator;
    return max(-viewZ, 0.0f);
}

float GetNormalizedVolumeDepth(float viewDepth)
{
    float safeNear = max(nearDistance, 0.001f);
    float safeFar = max(maxDistance, safeNear + 0.001f);
    float depth = clamp(viewDepth, safeNear, safeFar);

    return saturate(log(depth / safeNear) / log(safeFar / safeNear));
}

float4 SampleIntegratedVolume(float2 uv, float normalizedDepth)
{
    float depthInSlices = normalizedDepth * float(gridDepth);

    if (depthInSlices <= 1.0f)
    {
        float firstSliceZ = 0.5f / float(gridDepth);
        float4 firstSlice = integratedVolume.SampleLevel(linearClampSampler, float3(uv, firstSliceZ), 0.0f);
        return lerp(float4(0.0f, 0.0f, 0.0f, 1.0f), firstSlice, saturate(depthInSlices));
    }

    float volumeZ = (depthInSlices - 0.5f) / float(gridDepth);
    return integratedVolume.SampleLevel(linearClampSampler, float3(uv, saturate(volumeZ)), 0.0f);
}

float4 main(float4 position : SV_Position, float2 coord : TEXCOORD0) : SV_TARGET
{
    float deviceDepth = depthTexture.Load(int3(int2(position.xy), 0));
    float viewDepth = LinearizeViewDepth(deviceDepth);
    float normalizedDepth = GetNormalizedVolumeDepth(viewDepth);
    float4 integrated = SampleIntegratedVolume(coord, normalizedDepth);

    return float4(integrated.rgb, saturate(integrated.a));
}