#define MAX_DYNAMIC_TRANSPARENCY_TARGETS 2

cbuffer DynamicTransparencyMaskCB : register(b0)
{
    float4 centerRadius[MAX_DYNAMIC_TRANSPARENCY_TARGETS];
    float4 depthSoftness[MAX_DYNAMIC_TRANSPARENCY_TARGETS];
    float4 settings;
};

Texture2D<float> targetDepthTexture : register(t0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float2 main(PSInput input) : SV_Target0
{
    float exactTargetDepth = targetDepthTexture.Load(int3(int2(input.position.xy), 0));

    // CORE: preserve the exact target silhouette and its exact per-pixel depth.
    if (exactTargetDepth < 1.0f)
        return float2(1.0f, exactTargetDepth);

    uint targetCount = min((uint) settings.x, (uint) MAX_DYNAMIC_TRANSPARENCY_TARGETS);
    float maxFalloffInfluence = saturate(settings.y);

    float bestInfluence = 0.0f;
    float bestTargetDepth = 0.0f;

    for (uint i = 0; i < targetCount; ++i)
    {
        float2 center = centerRadius[i].xy;
        float2 radius = max(centerRadius[i].zw, float2(1.0f, 1.0f));

        float2 normalizedOffset = (input.position.xy - center) / radius;
        float radialDistance = length(normalizedOffset);

        if (radialDistance >= 1.0f)
            continue;

        float softness = saturate(depthSoftness[i].y);
        float edgeWidth = max(softness, 0.001f);
        float edgeStart = 1.0f - edgeWidth;

        float spatialInfluence = 1.0f - smoothstep(edgeStart, 1.0f, radialDistance);
        float falloffInfluence = spatialInfluence * maxFalloffInfluence;

        if (falloffInfluence > bestInfluence)
        {
            bestInfluence = falloffInfluence;
            bestTargetDepth = depthSoftness[i].x;
        }
    }

    return float2(bestInfluence, bestTargetDepth);
}