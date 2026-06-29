Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PSInput input) : SV_Target
{
    float4 tex = fontTexture.Sample(fontSampler, input.texCoord);
    return tex * input.color;
}