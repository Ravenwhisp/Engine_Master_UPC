#pragma once

#include <cstdint>
#include <functional>
#include <string>

enum class HapticCurve : uint8_t
{
    Linear,       // 1 - t  (fades linearly to silence)
    Exponential,  // exp(-5t) (fast initial drop, long tail)
    Sustain,      // always 1.0 (caller controls duration)
    Punch,        // (1-t)^3  (instant peak, sharp cubic fall)
    Custom,       // user-supplied lambda stored in customCurve
};

enum class HapticPriority : uint8_t
{
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
};

struct HapticChannelDef
{
    float leftMotor = 0.0f;
    float rightMotor = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
};

struct HapticEffectDefinition
{
    std::string id; 

    float durationSeconds = 0.2f; 
    float attackSeconds = 0.0f; 
    float releaseSeconds = 0.0f; 
    float delaySeconds = 0.0f;  

    HapticChannelDef peak;    

    HapticCurve curve = HapticCurve::Linear;
    HapticPriority priority = HapticPriority::Normal;

    std::function<float(float t)> customCurve;

    static HapticEffectDefinition makeImpact(float intensity = 1.0f, float duration = 0.12f, HapticPriority priority = HapticPriority::Normal);
    static HapticEffectDefinition makeContinuous(float leftIntensity = 0.3f, float rightIntensity = 0.15f, float duration = 1.0f, HapticPriority priority = HapticPriority::Low);
    static HapticEffectDefinition makeExplosion(float peakIntensity = 1.0f, float duration = 0.6f, HapticPriority priority = HapticPriority::High);
    static HapticEffectDefinition makeArrowshot(float intensity = 0.9f, HapticPriority priority = HapticPriority::High);
    static HapticEffectDefinition makeUIClick(HapticPriority priority = HapticPriority::Low);
    static HapticEffectDefinition makeEngineLoop(float intensity = 0.25f);
};