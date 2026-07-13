cbuffer CameraParams : register(b0)
{
    float4x4 vp;
};


cbuffer EmitterParams : register(b1)
{
    float2 uvScale;
};


struct ShaderParticleData
{
    float4x4 worldPosition;
    float4 colorAndAlpha;
    float2 sheetOffset;
};

StructuredBuffer<ShaderParticleData> instanceDataBuffer : register(t1);

struct VSOut
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float2 screenUV : SCREENPOS;
    uint instanceID : INSTANCEID;
};

VSOut main(float2 position : POSITION, float2 texCoord : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VSOut output;
    //output.texCoord = (texCoord - 0.5f) * uvScale + instanceDataBuffer[instanceID].sheetOffset;
    output.texCoord = texCoord * uvScale + instanceDataBuffer[instanceID].sheetOffset;
    //output.texCoord = texCoord;
    output.position = mul( mul(float4(position, 0.0f, 1.0f), instanceDataBuffer[instanceID].worldPosition), vp);
    output.instanceID = instanceID;
    
    // screenUV calculation (from clip space position)
    float normalizedDeviceCoord = output.position.xy / output.position.w;
    output.screenUV = normalizedDeviceCoord * 0.5f + 0.5f; // [-1, 1] -> [0, 1]
    
    return output;
}