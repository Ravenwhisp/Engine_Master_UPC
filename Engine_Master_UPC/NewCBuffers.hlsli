cbuffer SceneData : register(b1)
{
    float3 viewPos;
    float pad0;

    float2 screenSize;
    float2 invScreenSize;

    // x = ssaoEnabled
    // y = ssaoDebugView
    float4 renderFlags;
};

cbuffer ModelData : register(b2)
{
    float4x4 model;
    float4x4 normalMat;

    float3 baseColor;
    uint hasBaseColorTex;

    float metallicFactor;
    float roughnessFactor;
    uint hasMetallicRoughnessTex;
    
    float normalFactor;
    uint hasNormalTex;
    
    float3 emissiveColor;
    uint hasEmissiveTex;
    
    float3 padding;
};

#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 112
#define MAX_SPOT_LIGHTS 16

struct DirectionalLight
{
    float3 direction;
    float pad0;
    float3 color;
    float intensity;
};

struct PointLight
{
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct SpotLight
{
    float3 position;
    float radius;

    float3 direction;
    float pad0;

    float3 color;
    float intensity;

    float cosineInnerAngle;
    float cosineOuterAngle;
    float2 pad1;
};

cbuffer LightsCB : register(b3)
{
    float3 ambientColor;
    float ambientIntensity;

    uint directionalCount;
    uint pointCount;
    uint spotCount;
    uint paddingCounts;

    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

cbuffer ShadowData : register(b4)
{
    float4x4 lightViewProjection;
    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;
    
    //PCF
    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;
};