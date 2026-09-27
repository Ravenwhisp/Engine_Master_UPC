Texture2D<float4> shadowDiffuse : register(t0);
SamplerState shadowMaterialSampler : register(s0);
void main(float4 position : SV_POSITION, float2 uv : TEXCOORD0)
{
    clip(shadowDiffuse.Sample(shadowMaterialSampler, uv).a - 0.5f);
}
