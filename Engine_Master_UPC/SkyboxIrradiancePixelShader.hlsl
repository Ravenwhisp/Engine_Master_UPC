TextureCube skyTexture : register(t0);
SamplerState skySampler : register(s0);

float4 main(float3 direction : TEXCOORD0) : SV_TARGET
{
    return skyTexture.Sample(skySampler, normalize(direction));
}