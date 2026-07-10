#pragma once

#include <cstdint>
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

struct LightDefaults
{
    static constexpr float DEFAULT_INTENSITY = 1.0f;

    static constexpr float DEFAULT_POINT_RADIUS = 10.0f;

    static constexpr float DEFAULT_SPOT_RADIUS = 10.0f;
    static constexpr float DEFAULT_SPOT_INNER_ANGLE_DEGREES = 20.0f;
    static constexpr float DEFAULT_SPOT_OUTER_ANGLE_DEGREES = 30.0f;

    static constexpr float DEFAULT_AMBIENT_INTENSITY = 1.0f;
    static constexpr Vector3 DEFAULT_AMBIENT_COLOR = Vector3(0.2f, 0.2f, 0.2f);

    static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
    static constexpr uint32_t MAX_POINT_LIGHTS = 112;
    static constexpr uint32_t MAX_SPOT_LIGHTS = 16;

    static constexpr bool DEFAULT_CAST_SHADOWS = false;
    static constexpr uint32_t DEFAULT_SHADOW_MAP_SIZE = 4096;
    static constexpr uint32_t MIN_SHADOW_MAP_SIZE = 1024;
    static constexpr uint32_t MAX_SHADOW_MAP_SIZE = 8192;

    static constexpr bool DEFAULT_SHADOW_PCF_ENABLED = true;
    static constexpr uint32_t DEFAULT_SHADOW_PCF_RADIUS = 1; // 1 = 3x3, 2 = 5x5

    static constexpr float DEFAULT_SHADOW_BIAS = 0.0005f;
    static constexpr float DEFAULT_SHADOW_STRENGTH = 1.0f;

};

enum class LightType : uint8_t
{
    DIRECTIONAL = 0,
    POINT = 1,
    SPOT = 2,
    COUNT = 3
};

struct LightCommon
{
    Vector3 color = Vector3::One;
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct LightShadowSettings
{
    bool castShadows = LightDefaults::DEFAULT_CAST_SHADOWS;
    uint32_t shadowMapSize = LightDefaults::DEFAULT_SHADOW_MAP_SIZE;

    bool pcfEnabled = LightDefaults::DEFAULT_SHADOW_PCF_ENABLED;
    uint32_t pcfRadius = LightDefaults::DEFAULT_SHADOW_PCF_RADIUS;

    float shadowBias = LightDefaults::DEFAULT_SHADOW_BIAS;
    float shadowStrength = LightDefaults::DEFAULT_SHADOW_STRENGTH;
};

struct DirectionalLightParameters
{
};

struct PointLightParameters
{
    float radius = LightDefaults::DEFAULT_POINT_RADIUS;
};

struct SpotLightParameters
{
    float radius = LightDefaults::DEFAULT_SPOT_RADIUS;
    float innerAngleDegrees = LightDefaults::DEFAULT_SPOT_INNER_ANGLE_DEGREES;
    float outerAngleDegrees = LightDefaults::DEFAULT_SPOT_OUTER_ANGLE_DEGREES;
};

struct LightParameters
{
    DirectionalLightParameters directional{};
    PointLightParameters       point{};
    SpotLightParameters        spot{};

    static LightParameters makeDirectional()
    {
        LightParameters parameters{};
        parameters.directional = {};
        return parameters;
    }

    static LightParameters makePoint(float radius)
    {
        LightParameters parameters{};
        parameters.point = { radius };
        return parameters;
    }

    static LightParameters makeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees)
    {
        LightParameters parameters{};
        parameters.spot = { radius, innerAngleDegrees, outerAngleDegrees };
        return parameters;
    }
};

struct LightData
{
    LightCommon common{};
    LightShadowSettings shadow{};
    LightType type = LightType::DIRECTIONAL;
    LightParameters parameters = LightParameters::makeDirectional();
};

struct GPUDirectionalLight
{
    Vector3 direction{};
    float padding0 = 0.0f;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct GPUPointLight
{
    Vector3 position{};
    float radius = LightDefaults::DEFAULT_POINT_RADIUS;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;
};

struct GPUSpotLight
{
    Vector3 position{};
    float radius = LightDefaults::DEFAULT_SPOT_RADIUS;

    Vector3 direction{};
    float padding0 = 0.0f;

    Vector3 color{};
    float intensity = LightDefaults::DEFAULT_INTENSITY;

    float cosineInnerAngle = 0.0f;
    float cosineOuterAngle = 0.0f;
    float padding1[2] = { 0.0f, 0.0f };
};

struct GPULightsConstantBuffer
{
    Vector3 ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    float ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;
    uint32_t spotCount = 0;
    uint32_t paddingCounts = 0;

    GPUDirectionalLight directionalLights[LightDefaults::MAX_DIRECTIONAL_LIGHTS];
    GPUPointLight pointLights[LightDefaults::MAX_POINT_LIGHTS];
    GPUSpotLight spotLights[LightDefaults::MAX_SPOT_LIGHTS];
};