cbuffer RevealSettings : register(b1)
{
    float depthBias;
};

Texture2D<float> mainDepth : register(t0);
Texture2D<float> occluderEligibility : register(t1);

struct PSInput
{
    float4 position : SV_POSITION;
};

float4 main(PSInput input) : SV_Target0
{
    int2 pixel = int2(input.position.xy);

    float sceneDepth = mainDepth.Load(int3(pixel, 0));
    float eligibility = occluderEligibility.Load(int3(pixel, 0));
    float playerDepth = input.position.z;

    if (eligibility <= 0.5f)
        discard;

    if (playerDepth <= sceneDepth + depthBias)
        discard;

    return float4(1.0f, 0.0f, 1.0f, 1.0f);
}