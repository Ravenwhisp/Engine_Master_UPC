Texture2D particleTexture : register(t0);
SamplerState particleSampler : register(s0);

Texture2D depthStencilTexture : register(t2);
SamplerState depthStencilSampler : register(s1);

// ALL THIS STUFF SHOULD GO ON A SEPARATE FILE (StructuredBuffer probably on an independent one, the other is shared with other shaders)

struct ShaderParticleData
{
    float4x4 worldPosition;
    float4 colorAndAlpha;
    float2 sheetOffset;
};

StructuredBuffer<ShaderParticleData> instanceDataBuffer : register(t1);

cbuffer CameraParams : register(b0)
{
    float4x4 vp;
    float2 depthlinearize; // for linearizing positions in the pixel shader (near, far, for soft particles)
};


static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;

static const float softParticleFadeDistance = 0.5f; // for particles softness 

struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float2 screenUV : SCREENPOS;
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

float linearizeDepth(float ndcDepth)
{
    return depthlinearize.y / (ndcDepth + depthlinearize.x);
}

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = particleTexture.Sample(particleSampler, input.texCoord);
    clip(texColor.a - 0.001f);
    
    // Soft particles calculation //
    
    float sceneDepth = linearizeDepth( depthStencilTexture.Sample(depthStencilSampler, input.screenUV).r ); //
    float particleDepth = linearizeDepth(input.position.z);                                                 // (all where in NDC)
    
    float depthDiff = sceneDepth - particleDepth;
    
    //if (depthDiff < 0.0f) // => behind the geometry (TO REVISE)
    //{
    //    discard;
    //}
    
    // Softness: the closer to the geometry, the more it disappears
    float fade = saturate(depthDiff / softParticleFadeDistance);
    
    // Color application //
    
    float4 particleColorInfo = instanceDataBuffer[input.instanceID].colorAndAlpha;
    float3 resultingColor = LinearToSRGB(Tint(texColor.rgb, particleColorInfo.rgb) );
    
    return float4(resultingColor, fade * texColor.a * particleColorInfo.a);
}