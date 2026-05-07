cbuffer SceneData : register(b0)
{
    float3 viewPos;
    float pad0;
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

cbuffer LightsCB : register(b1)
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