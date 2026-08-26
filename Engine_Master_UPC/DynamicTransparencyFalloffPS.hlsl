cbuffer FalloffCB : register(b1)
{
    float4 baseColorDepthBias;
    float4 coverage;
};

Texture2D diffuseTex : register(t0);
Texture2D dissolveNoise : register(t8);
Texture2D<float2> dynamicTransparencyMask : register(t9);

SamplerState linearWrapSample : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
    int2 pixelCoord = int2(input.position.xy);
    float2 transparencyData = dynamicTransparencyMask.Load(int3(pixelCoord, 0));

    float influence = transparencyData.r;
    float targetDepth = transparencyData.g;

    const float coreThreshold = 0.999f;
    float depthBias = baseColorDepthBias.w;

    // OUTSIDE: the normal opaque GBuffer wall already exists here.
    if (influence <= 0.0f)
        discard;

    // CORE: this MUST remain completely free of wall.
    if (influence >= coreThreshold)
        discard;

    // Only render an occluder that is actually in front of the target.
    if (input.position.z >= targetDepth - depthBias)
        discard;

    float3 albedo = baseColorDepthBias.rgb;

    bool hasDiffuseTex = coverage.x > 0.5f;
    bool hasDissolveComponent = coverage.y > 0.5f;
    float dissolveAmount = coverage.z;

    if (hasDiffuseTex)
    {
        float4 diffuseSample = diffuseTex.Sample(linearWrapSample, input.texCoord);

        if (diffuseSample.a < 0.5f)
            discard;

        albedo *= diffuseSample.rgb;
    }

    if (hasDissolveComponent && dissolveAmount > 0.0f)
    {
        float noise = dissolveNoise.Sample(linearWrapSample, input.texCoord).r;

        if (noise <= dissolveAmount)
            discard;
    }

    float wallOpacity = saturate(1.0f - influence);

    return float4(albedo, wallOpacity);
}