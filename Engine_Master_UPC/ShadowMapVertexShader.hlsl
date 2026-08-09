#define MAX_SHADOW_CASCADES 4
#define FULL_FRUSTUM_MATRIX_INDEX 4

cbuffer ShadowDrawData : register(b0)
{
    float4x4 model;
};

cbuffer ShadowData : register(b1)
{
    float4x4 lightViewProjection;

    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;

    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;

    uint cascadeCount;
    uint cascadeFitMode;
    float2 cascadePadding;

    float4 cascadeFarDistances;

    float4x4 cascadeLightViewProjection[MAX_SHADOW_CASCADES];
};

cbuffer ShadowRenderParams : register(b2)
{
    uint shadowMatrixIndex;
};

float4x4 GetShadowViewProjection()
{
    if (shadowMatrixIndex == 0)
    {
        return cascadeLightViewProjection[0];
    }

    if (shadowMatrixIndex == 1)
    {
        return cascadeLightViewProjection[1];
    }

    if (shadowMatrixIndex == 2)
    {
        return cascadeLightViewProjection[2];
    }

    if (shadowMatrixIndex == 3)
    {
        return cascadeLightViewProjection[3];
    }

    return lightViewProjection;
}

float4 main(float3 position : POSITION) : SV_POSITION
{
    float4 worldPosition =
        mul(
            float4(position, 1.0f),
            model);

    float4x4 shadowViewProjection =
        GetShadowViewProjection();

    return mul(
        worldPosition,
        shadowViewProjection);
}