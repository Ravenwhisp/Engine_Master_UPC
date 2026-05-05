cbuffer CameraParams : register(b0)
{
    float4x4 vp;
};

struct ShaderParticleData
{
    float4x4 worldPosition;
    float4 colorAndAlpha;
};

StructuredBuffer<ShaderParticleData> instanceDataBuffer : register(t1);

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VSOut output;
    output.texCoord = texCoord;

    float4 pos = float4(position.xy, 0.0f, 1.0f);

    output.position = mul(mul(pos, instanceDataBuffer[instanceID].worldPosition), vp);
    
    output.color = instanceDataBuffer[instanceID].colorAndAlpha;
    
    return output;
}