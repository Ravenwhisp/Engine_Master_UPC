cbuffer CoverageCB : register(b1)
{
    uint hasDiffuseTex;
    uint hasDissolveComponent;
    float dissolveAmount;
    float padding;
};

Texture2D diffuseTex : register(t0);
Texture2D dissolveNoise : register(t8);

SamplerState linearWrapSample : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

void main(PSInput input)
{
    if (hasDiffuseTex != 0)
    {
        float alpha = diffuseTex.Sample(
            linearWrapSample,
            input.texCoord
        ).a;

        if (alpha < 0.5f)
        {
            discard;
        }
    }

    if (hasDissolveComponent != 0 &&
        dissolveAmount > 0.0f)
    {
        float noise = dissolveNoise.Sample(
            linearWrapSample,
            input.texCoord
        ).r;

        if (noise <= dissolveAmount)
        {
            discard;
        }
    }
}