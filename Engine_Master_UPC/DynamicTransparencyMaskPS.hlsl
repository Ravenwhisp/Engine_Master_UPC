Texture2D<float> targetDepthTexture : register(t0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float2 main(PSInput input) : SV_Target0
{
    float targetDepth = targetDepthTexture.Load(int3(int2(input.position.xy), 0));

    if (targetDepth >= 1.0f)
        return float2(0.0f, 0.0f);

    return float2(1.0f, targetDepth);
}