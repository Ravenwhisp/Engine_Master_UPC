#ifndef GBUFFER_COMMON_HLSLI
#define GBUFFER_COMMON_HLSLI

// b0 — root 32-bit constants (GeometryPass)
cbuffer MVCB : register(b0)
{
    float4x4 gMVP;          // (model * view * proj).Transpose() from CPU
};

// b1 — SceneDataCB
cbuffer SceneDataCB : register(b1)
{
    float3 gViewPos;
    float  _pad0;
};

// b2 — per-object data (GeometryPass)
struct MaterialData
{
    float3 diffuseColour;
    bool hasDiffuseTex;

    float metallicFactor;
    float roughnessFactor;
    bool hasMetallicRoughnessTex;
    
    float normalFactor;
    uint hasNormalTex;
    
    float3 emissiveColor;
    uint hasEmissiveTex;

    float3 padding;
};

cbuffer ModelDataCB : register(b2)
{
    float4x4     gModel;        // world matrix, pre-transposed
    float4x4     gNormalMat;    // inverse-transpose world, pre-transposed
    MaterialData gMaterial;
};

// b3 — lights (LightingPass)
#ifndef MAX_DIR_LIGHTS
#define MAX_DIR_LIGHTS    4
#endif
#ifndef MAX_POINT_LIGHTS
#define MAX_POINT_LIGHTS  32
#endif
#ifndef MAX_SPOT_LIGHTS
#define MAX_SPOT_LIGHTS   8
#endif

struct DirectionalLightData
{
    float3 direction;   float  _pad;
    float3 color;       float  intensity;
};

struct PointLightData
{
    float3 position;    float  radius;
    float3 color;       float  intensity;
};

struct SpotLightData
{
    float3 position;    float  radius;
    float3 direction;   float  cosInner;
    float3 color;       float  cosOuter;
    float  intensity;   float3 _pad;
};

cbuffer LightsCB : register(b3)
{
    float3 gAmbientColor;
    float  gAmbientIntensity;
    int    gDirCount;
    int    gPointCount;
    int    gSpotCount;
    int    _lpad;
    DirectionalLightData gDirLights  [MAX_DIR_LIGHTS];
    PointLightData       gPointLights[MAX_POINT_LIGHTS];
    SpotLightData        gSpotLights [MAX_SPOT_LIGHTS];
};


float3 CalcAmbient(float3 albedo)
{
    return gAmbientColor * gAmbientIntensity * albedo;
}

float3 CalcDirectional(DirectionalLightData light, float3 N, float3 V, float3 albedo, float3 specColor, float smoothness)
{
    float3 L       = normalize(-light.direction);
    float  NdotL   = max(dot(N, L), 0.0f);
    float3 H       = normalize(L + V);
    float  specPow = pow(2.0f, smoothness * 10.0f) + 1.0f;
    float  spec    = pow(max(dot(N, H), 0.0f), specPow) * NdotL;
    return (albedo * NdotL + specColor * spec) * (light.color * light.intensity);
}

float3 CalcPoint(PointLightData light, float3 worldPos, float3 N, float3 V, float3 albedo, float3 specColor, float smoothness)
{
    float3 toLight = light.position - worldPos;
    float  dist    = length(toLight);
    if (dist > light.radius) return 0.0f;

    float3 L       = toLight / dist;
    float  NdotL   = max(dot(N, L), 0.0f);
    float3 H       = normalize(L + V);
    float  specPow = pow(2.0f, smoothness * 10.0f) + 1.0f;
    float  spec    = pow(max(dot(N, H), 0.0f), specPow) * NdotL;
    float  atten   = 1.0f - saturate(dist / light.radius);
    atten         *= atten;
    return (albedo * NdotL + specColor * spec) * (light.color * light.intensity * atten);
}

float3 CalcSpot(SpotLightData light, float3 worldPos, float3 N, float3 V,float3 albedo, float3 specColor, float smoothness)
{
    float3 toLight  = light.position - worldPos;
    float  dist     = length(toLight);
    if (dist > light.radius) return 0.0f;

    float3 L        = toLight / dist;
    float  cosAngle = dot(-L, normalize(light.direction));
    float  spotMask = saturate((cosAngle - light.cosOuter) /
                               max(light.cosInner - light.cosOuter, 1e-5f));
    if (spotMask <= 0.0f) return 0.0f;

    float  NdotL   = max(dot(N, L), 0.0f);
    float3 H       = normalize(L + V);
    float  specPow = pow(2.0f, smoothness * 10.0f) + 1.0f;
    float  spec    = pow(max(dot(N, H), 0.0f), specPow) * NdotL;
    float  atten   = 1.0f - saturate(dist / light.radius);
    atten         *= atten * spotMask;
    return (albedo * NdotL + specColor * spec) * (light.color * light.intensity * atten);
}

#endif // GBUFFER_COMMON_HLSLI
