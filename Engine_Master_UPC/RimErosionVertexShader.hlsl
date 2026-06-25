#include "NewCBuffers.hlsli"

cbuffer Transforms : register(b0)
{
    float4x4 mvp;
    float4x4 nm;
};

cbuffer ErosionData : register(b5)
{
    float displacementAmount;
    float rimThreshold;
    float rimSoftness;
    float erosionIntensity;
    float brushScale;
    float brushOffsetX;
    float brushOffsetY;
    float preserveSilhouette;
    float4 erosionColor;
    float debugRimMask;
    float3 eroPad;
    float4 paintColor1;
    float4 paintColor2;
    float brushNormalStrength;
    float curvatureScale;
    float toonSharpness;
    float pad3;
};

struct VertexOutput
{
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
    float2 brushUV : BRUSHUV;
    float rimFactor : RIMFACTOR;
    float4 position : SV_POSITION;
};

VertexOutput main(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL, float3 tangent : TANGENT)
{
    VertexOutput output;

    float3 worldPos = mul(float4(position, 1.0), model).xyz;
    output.normal = normalize(mul(normal, (float3x3) normalMat));
    output.tangent = normalize(mul(tangent, (float3x3) normalMat));
    output.texCoord = texCoord;
    output.brushUV = texCoord * brushScale + float2(brushOffsetX, brushOffsetY);

    float3 viewDir = normalize(viewPos - worldPos);
    float NdotV = saturate(dot(output.normal, viewDir));
    output.rimFactor = 1.0 - NdotV;
    output.worldPos = worldPos;

    // Inward displacement at rim for erosion
    float rimMask = smoothstep(rimThreshold - rimSoftness, rimThreshold + rimSoftness, output.rimFactor);
    float3 displacedPos = position - normal * displacementAmount * rimMask;
    output.position = mul(float4(displacedPos, 1.0f), mvp);

    return output;
}
