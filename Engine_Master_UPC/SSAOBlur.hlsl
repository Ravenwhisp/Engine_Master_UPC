Texture2D ssaoTexture : register(t0);
SamplerState pointClampSampler : register(s0);

cbuffer SSAOBlurData : register(b0)
{
    float4 screenParams;
    // x = width
    // y = height
    // z = 1 / width
    // w = 1 / height
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PixelInput input) : SV_TARGET
{
    float2 texelSize = screenParams.zw;

    float result = 0.0f;

    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(-1.0f, -1.0f), 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(0.0f, -1.0f), 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(1.0f, -1.0f), 0).r;

    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(-1.0f, 0.0f), 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv, 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(1.0f, 0.0f), 0).r;

    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(-1.0f, 1.0f), 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(0.0f, 1.0f), 0).r;
    result += ssaoTexture.SampleLevel(pointClampSampler, input.uv + texelSize * float2(1.0f, 1.0f), 0).r;

    result /= 9.0f;

    return float4(result, result, result, 1.0f);
}