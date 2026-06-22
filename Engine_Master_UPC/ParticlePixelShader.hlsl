Texture2D particleTexture : register(t0);
SamplerState particleSampler : register(s0);

// ALL THIS STUFF SHOULD GO ON A SEPARATE FILE (StructuredBuffer probably on an independent one, the other is shared with other shaders)

struct ShaderParticleData
{
    float4x4 worldPosition;
    float4 colorAndAlpha;
    float2 sheetOffset;
};

StructuredBuffer<ShaderParticleData> instanceDataBuffer : register(t1);

static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;
static const float TWO_PI = 6.28318530f;

struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    uint instanceID : INSTANCEID;
};

float3 LinearToSRGB(float3 color)
{
    color = max(color, 0.0f);
    return pow(color, INV_GAMMA);
}

float3 Tint(float3 input, float3 color)
{
    return input * color;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = particleTexture.Sample(particleSampler, input.texCoord);
    clip(texColor.a - 0.001f);
    
    float4 particleColorInfo = instanceDataBuffer[input.instanceID].colorAndAlpha;
    float3 resultingColor = Tint(texColor.rgb, particleColorInfo.rgb);

    return float4(resultingColor, texColor.a * particleColorInfo.a);
}