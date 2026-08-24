struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 color : COLOR;
};

cbuffer GradientConstantBuffer : register(b1)
{
    uint hasTexture;
    float3 padding;
    
};

Texture2D texture : register(t8);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);

float4 main(PSInput input) : SV_TARGET
{
    float4 result = input.color;
    
    if (hasTexture != 0)
    {
        float4 texSample = texture.Sample(linearWrapSample, input.texCoord);
        
        result *= texSample;
    }
    
    return result;
}