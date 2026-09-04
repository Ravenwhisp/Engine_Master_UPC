#define MAX_OCCLUSION_BUBBLES 2

cbuffer BubbleCB : register(b0)
{
    float4 bubbleCenterRadius[MAX_OCCLUSION_BUBBLES];
    float4 bubbleDepthParams[MAX_OCCLUSION_BUBBLES];
    float4 bubbleSettings;
};

Texture2D<float> mainDepth : register(t0);
Texture2D<float> occluderEligibility : register(t1);

float4 main(float4 position : SV_POSITION) : SV_Target0
{
    int2 pixel = int2(position.xy);

    float eligibility = occluderEligibility.Load(int3(pixel, 0));

    if (eligibility <= 0.5f)
        discard;

    float sceneDepth = mainDepth.Load(int3(pixel, 0));
    float destinationScale = 1.0f;
    bool affected = false;

    for (uint i = 0; i < MAX_OCCLUSION_BUBBLES; ++i)
    {
        float4 centerRadius = bubbleCenterRadius[i];
        float4 depthParams = bubbleDepthParams[i];

        float enabled = depthParams.w;

        if (enabled < 0.5f)
            continue;

        float targetDepth = depthParams.x;
        float softness = saturate(depthParams.y);
        float occluderOpacity = saturate(depthParams.z);

        if (sceneDepth + bubbleSettings.x >= targetDepth)
            continue;

        float2 radius = max(centerRadius.zw, float2(1.0f, 1.0f));
        float2 normalizedOffset = (position.xy - centerRadius.xy) / radius;

        float distanceFromCenter = length(normalizedOffset);

        if (distanceFromCenter >= 1.0f)
            continue;

        float innerEdge = max(0.0f, 1.0f - softness);
        float bubbleMask = 1.0f - smoothstep(innerEdge, 1.0f, distanceFromCenter);

        float localScale = lerp(1.0f, occluderOpacity, bubbleMask);

        destinationScale = min(destinationScale, localScale);
        affected = true;
    }

    if (!affected)
        discard;

    // RGB is ignored by the blend state.
    // Alpha controls how much of the existing occluder colour remains.
    return float4(0.0f, 0.0f, 0.0f, destinationScale);
}