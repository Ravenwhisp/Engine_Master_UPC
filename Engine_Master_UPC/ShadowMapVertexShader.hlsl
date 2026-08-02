cbuffer ShadowDrawData : register(b0)
{
    float4x4 model;
};

cbuffer ShadowViewProjection : register(b1)
{
    float4x4 lightViewProjection;
};

float4 main(float3 position : POSITION) : SV_POSITION
{
    float4 worldPosition =
        mul(float4(position, 1.0f), model);

    return mul(
        worldPosition,
        lightViewProjection);
}