Texture2D videoTexture : register(t0);
SamplerState videoSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return videoTexture.Sample(videoSampler, input.uv);
}