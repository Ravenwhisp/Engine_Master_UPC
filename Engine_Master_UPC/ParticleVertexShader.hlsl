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
    uint instanceID : INSTANCEID;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VSOut output;
    output.texCoord = texCoord;
    output.position = mul( mul(float4(position, 0.0f, 1.0f), instanceDataBuffer[instanceID].worldPosition), vp);
    output.instanceID = instanceID;
    
    return output;
}