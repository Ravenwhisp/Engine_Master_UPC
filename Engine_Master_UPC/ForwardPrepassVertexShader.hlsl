#include "ForwardCommon.hlsli"

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.position = mul(float4(input.position, 1.0f), mvp);

    return output;
}