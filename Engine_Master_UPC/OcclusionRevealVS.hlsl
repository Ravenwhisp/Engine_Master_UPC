cbuffer MvpCB : register(b0)
{
    float4x4 gMVP;
};

cbuffer ModelDataCB : register(b4)
{
    float4x4 gModel;
    float4x4 gNormalMat;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 clipPos : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.worldPos = mul(float4(input.position, 1.0f), gModel).xyz;
    output.clipPos = mul(float4(input.position, 1.0f), gMVP);
    output.normal = normalize(mul(input.normal, (float3x3) gNormalMat));
    output.tangent = normalize(mul(input.tangent, (float3x3) gNormalMat));
    output.texCoord = input.texCoord;

    return output;
}