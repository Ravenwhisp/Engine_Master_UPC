Texture2D    inputTexture  : register(t0);
SamplerState bilinearClamp : register(s0);

cbuffer BloomParams : register(b0)
{
    float threshold;
    float pad0;
    float pad1;
    float pad2;
};

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    float3 c = inputTexture.Sample(bilinearClamp, uv).rgb;

    float brightness = max(c.r, max(c.g, c.b));
    float contribution = max(brightness - threshold, 0.0) / max(brightness, 1e-5);

    return float4(c * contribution, 1.0);
}
