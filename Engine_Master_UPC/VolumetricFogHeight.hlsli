#ifndef VOLUMETRIC_FOG_HEIGHT_HLSLI
#define VOLUMETRIC_FOG_HEIGHT_HLSLI

// Average of exp(-falloff * max(y - baseHeight, 0)) along a straight segment.
// Split at baseHeight: the artist's constant-density region below it is retained.
float FogHeightAverage(float y0, float y1, float baseHeight, float falloff)
{
    float k = max(falloff, 0.0f);
    float low = min(y0, y1) - baseHeight;
    float high = max(y0, y1) - baseHeight;
    float span = high - low;
    if (span < 0.00001f)
        return exp(-k * max(0.5f * (low + high), 0.0f));
    if (high <= 0.0f)
        return 1.0f;

    float aboveStart = max(low, 0.0f);
    float aboveLength = high - aboveStart;
    float x = k * aboveLength;
    float exponentialAverage = x < 0.01f
        ? 1.0f - x * 0.5f + x * x / 6.0f - x * x * x / 24.0f
        : (1.0f - exp(-x)) / x;
    float belowLength = max(-low, 0.0f);
    return (belowLength + aboveLength * exp(-k * aboveStart) * exponentialAverage) / span;
}

float FogAbsorbedFraction(float opticalDepth)
{
    float x = max(opticalDepth, 0.0f);
    return x < 0.01f ? x * (1.0f - x * 0.5f + x * x / 6.0f) : 1.0f - exp(-x);
}
#endif
