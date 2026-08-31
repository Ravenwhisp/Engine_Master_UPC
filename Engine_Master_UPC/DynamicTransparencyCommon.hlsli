#ifndef DYNAMIC_TRANSPARENCY_COMMON_HLSLI
#define DYNAMIC_TRANSPARENCY_COMMON_HLSLI

#define MAX_DYNAMIC_TRANSPARENCY_TARGETS 2

static const float DYNAMIC_TRANSPARENCY_CORE_THRESHOLD = 0.999f;

float EvaluateDynamicTransparencyInfluence(float4 transparencyData, float fragmentDepth, float depthBias)
{
    float influence = 0.0f;

    if (transparencyData.x > 0.0f && fragmentDepth < transparencyData.y - depthBias)
        influence = max(influence, transparencyData.x);
    if (transparencyData.z > 0.0f && fragmentDepth < transparencyData.w - depthBias)
        influence = max(influence, transparencyData.z);

    return influence;
}

#endif