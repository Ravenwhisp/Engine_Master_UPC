#define SSAO_KERNEL_SIZE 32
#define PI 3.14159265359f

Texture2D depthTexture : register(t0);
Texture2D normalTexture : register(t1);

SamplerState pointClampSampler : register(s0);

cbuffer SSAOData : register(b0)
{
    float4x4 projection;
    float4x4 inverseProjection;

    float4 samples[SSAO_KERNEL_SIZE];

    // x = radius
    // y = bias
    // z = strength
    // w = sampleCount
    float4 ssaoParams;

    // x = width
    // y = height
    // z = 1 / width
    // w = 1 / height
    float4 screenParams;

    // x = frameIndex
    // yzw = unused
    float4 frameParams;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float3 DecodeNormal(float3 encodedNormal)
{
    return normalize(encodedNormal * 2.0f - 1.0f);
}

float3 ReconstructViewPosition(float2 uv, float depth)
{
    float4 clipPosition;
    clipPosition.x = uv.x * 2.0f - 1.0f;
    clipPosition.y = 1.0f - uv.y * 2.0f;
    clipPosition.z = depth;
    clipPosition.w = 1.0f;

    float4 viewPosition = mul(clipPosition, inverseProjection);
    return viewPosition.xyz / viewPosition.w;
}

float2 NdcToUV(float2 ndc)
{
    return float2(
        ndc.x * 0.5f + 0.5f,
        -ndc.y * 0.5f + 0.5f
    );
}

float SampleIGN(float2 pixelXY, float frameIndex)
{
    pixelXY += frameIndex * 5.588238f;
    return frac(52.9829189f * frac(0.06711056f * pixelXY.x + 0.00583715f * pixelXY.y));
}

float3 GetRandomTangent(float2 pixelXY, float frameIndex)
{
    float randomAngle = SampleIGN(pixelXY, frameIndex) * 2.0f * PI;
    return float3(cos(randomAngle), sin(randomAngle), 0.0f);
}

float3x3 ComputeTangentSpace(float3 normal, float3 randomTangent)
{
    float3 tangent = normalize(randomTangent - normal * dot(randomTangent, normal));
    float3 bitangent = cross(normal, tangent);

    return float3x3(tangent, bitangent, normal);
}

float4 main(PixelInput input) : SV_TARGET
{
    float depth = depthTexture.SampleLevel(pointClampSampler, input.uv, 0).r;

    if (depth >= 0.999999f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float3 normal = DecodeNormal(normalTexture.SampleLevel(pointClampSampler, input.uv, 0).rgb);
    float3 viewPos = ReconstructViewPosition(input.uv, depth);

    float radius = ssaoParams.x;
    float bias = ssaoParams.y;
    float strength = ssaoParams.z;
    int sampleCount = min((int) ssaoParams.w, SSAO_KERNEL_SIZE);

    float3 randomTangent = GetRandomTangent(input.position.xy, frameParams.x);
    float3x3 tangentSpace = ComputeTangentSpace(normal, randomTangent);

    float occlusion = 0.0f;
    float validSamples = 0.0f;

    for (int i = 0; i < sampleCount; ++i)
    {
        float3 sampleOffset = mul(samples[i].xyz, tangentSpace) * radius;
        float3 sampleViewPos = viewPos + sampleOffset;

        float4 sampleClipPos = mul(float4(sampleViewPos, 1.0f), projection);
        sampleClipPos.xyz /= sampleClipPos.w;

        float2 sampleUV = NdcToUV(sampleClipPos.xy);

        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f ||
            sampleUV.y < 0.0f || sampleUV.y > 1.0f ||
            sampleClipPos.z < 0.0f || sampleClipPos.z > 1.0f)
        {
            continue;
        }

        float sampleDepth = depthTexture.SampleLevel(pointClampSampler, sampleUV, 0).r;

        if (sampleDepth >= 0.999999f)
        {
            continue;
        }

        float3 sampleSceneViewPos = ReconstructViewPosition(sampleUV, sampleDepth);

        float depthDistance = abs(viewPos.z - sampleSceneViewPos.z);
        float rangeCheck = smoothstep(0.0f, 1.0f, radius / max(depthDistance, 0.0001f));

        float isOccluded = sampleDepth < sampleClipPos.z - bias ? 1.0f : 0.0f;

        occlusion += isOccluded * rangeCheck;
        validSamples += 1.0f;
    }

    if (validSamples <= 0.0f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float ao = 1.0f - (occlusion / validSamples);
    ao = saturate(lerp(1.0f, ao, strength));

    return float4(ao, ao, ao, 1.0f);
}