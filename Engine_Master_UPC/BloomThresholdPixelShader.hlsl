Texture2D    inputTexture  : register(t0);
SamplerState bilinearClamp : register(s0);

cbuffer BloomParams : register(b0)
{
    float threshold;
    float maxBrightness;
    float pad1;
    float pad2;
};

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    // Clamp the input so an extremely bright (or Inf) HDR pixel can't push the
    // 16-bit bloom chain up to Inf during the additive upsample (that would
    // tone-map to NaN and show up as a black blob) and so very intense lights
    // don't bloom into huge blown-out discs.
    float3 c = min(inputTexture.Sample(bilinearClamp, uv).rgb, maxBrightness);

    float brightness = max(c.r, max(c.g, c.b));
    float contribution = max(brightness - threshold, 0.0) / max(brightness, 1e-5);

    return float4(c * contribution, 1.0);
}
