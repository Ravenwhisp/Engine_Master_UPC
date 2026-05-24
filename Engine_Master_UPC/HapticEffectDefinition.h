#pragma once

#include "EngineAPI.h"
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

    enum class HeartbeatVariant : uint8_t
    {
        Health,    
        Separation,  
    };

    ENGINE_API static HapticEffectDefinition makeHeartbeatLub(float intensity = 0.5f, HeartbeatVariant variant = HeartbeatVariant::Health);

    ENGINE_API static HapticEffectDefinition makeHeartbeatDub(float intensity = 0.5f, HeartbeatVariant variant = HeartbeatVariant::Health);
};

struct HeartbeatCycle
{
    float intensity = 0.0f; 
    float diastoleSeconds = 0.5f; 
    float interBeatSeconds = 0.08f; 
    float dubScale = 0.65f; 

    float totalCycleSeconds() const
    {
        return 0.08f + interBeatSeconds + 0.08f + diastoleSeconds;
    }

    static HeartbeatCycle fromHealth(float healthNormalized)
    {
        const float h = healthNormalized < 0.0f ? 0.0f : (healthNormalized > 1.0f ? 1.0f : healthNormalized);
        const float danger = 1.0f - h; 

        HeartbeatCycle c;

        c.intensity = danger * danger * (danger < 1.0f ? std::sqrt(danger) : 1.0f); 

        const float bpm = 50.0f + 90.0f * danger * danger;
        const float cycleSeconds = 60.0f / bpm;

        const float beatsTime = 0.08f + 0.08f + 0.08f; 
        c.diastoleSeconds = cycleSeconds - beatsTime;
        if (c.diastoleSeconds < 0.03f) c.diastoleSeconds = 0.03f; 

        c.interBeatSeconds = 0.08f; 
        c.dubScale = (h < 0.15f) ? 0.80f : 0.65f;

        return c;
    }

    static HeartbeatCycle fromSeparation(float distanceNormalized, unsigned int seed = 0)
    {
        const float d = distanceNormalized < 0.0f ? 0.0f : (distanceNormalized > 1.0f ? 1.0f : distanceNormalized);

        HeartbeatCycle c;

        if (d <= 0.20f)
        {
            c.intensity = 0.0f;
            c.diastoleSeconds = 999.0f; 
            return c;
        }

        const float t = d - 0.20f; 

        c.intensity = 0.45f * (t * t) / (0.8f * 0.8f);

        const float bpm = 88.0f * (t / 0.8f);
        const float cycleSeconds = (bpm > 1.0f) ? (60.0f / bpm) : 60.0f;

        const unsigned int rng = (seed * 1664525u + 1013904223u);
        const float jitter = static_cast<float>(rng & 0xFF) / 255.0f; 
        c.interBeatSeconds = 0.060f + jitter * 0.050f; 

        const float beatsTime = 0.08f + c.interBeatSeconds + 0.08f;
        c.diastoleSeconds = cycleSeconds - beatsTime;
        if (c.diastoleSeconds < 0.03f) c.diastoleSeconds = 0.03f;

        c.dubScale = 0.55f; 

        return c;
    }
};