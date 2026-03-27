Texture2D uiTexture : register(t0);
SamplerState uiSampler : register(s0);

static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;

struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
    float4 position : SV_POSITION;
};

float3 LinearToSRGB(float3 color)
{
    color = max(color, 0.0f);
    return pow(color, INV_GAMMA);
}

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = uiTexture.Sample(uiSampler, input.texCoord);
    float4 linearColor = texColor * input.color;
    return float4(LinearToSRGB(linearColor.rgb), linearColor.a);
}