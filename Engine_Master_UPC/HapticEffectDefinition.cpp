#include "Globals.h"
#include "HapticEffectDefinition.h"

HapticEffectDefinition HapticEffectDefinition::makeImpact(float intensity, float duration, HapticPriority priority)
{
    HapticEffectDefinition def;
    def.id = "Impact";
    def.durationSeconds = duration;
    def.attackSeconds = 0.005f;
    def.curve = HapticCurve::Punch;
    def.priority = priority;
    def.peak.leftMotor = intensity;
    def.peak.rightMotor = intensity;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeContinuous(float leftIntensity, float rightIntensity, float duration, HapticPriority priority)
{
    HapticEffectDefinition def;
    def.id = "Continuous";
    def.durationSeconds = duration;
    def.curve = HapticCurve::Sustain;
    def.priority = priority;
    def.peak.leftMotor = leftIntensity;
    def.peak.rightMotor = rightIntensity;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeExplosion(float peakIntensity, float duration, HapticPriority priority)
{
    HapticEffectDefinition def;
    def.id = "Explosion";
    def.durationSeconds = duration;
    def.attackSeconds = 0.02f;
    def.releaseSeconds = 0.15f;
    def.curve = HapticCurve::Exponential;
    def.priority = priority;
    def.peak.leftMotor = peakIntensity;
    def.peak.rightMotor = peakIntensity * 0.5f;
    def.peak.leftTrigger = peakIntensity * 0.3f;
    def.peak.rightTrigger = peakIntensity * 0.3f;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeArrowshot(float intensity, HapticPriority priority)
{
    HapticEffectDefinition def;
    def.id = "ArrowShot";
    def.durationSeconds = 0.08f;
    def.attackSeconds = 0.005f;
    def.curve = HapticCurve::Punch;
    def.priority = priority;
    def.peak.leftMotor = intensity * 0.9f;
    def.peak.rightMotor = intensity * 0.7f;
    def.peak.leftTrigger = intensity * 0.8f;
    def.peak.rightTrigger = intensity * 0.1f;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeUIClick(HapticPriority priority)
{
    HapticEffectDefinition def;
    def.id = "UIClick";
    def.durationSeconds = 0.04f;
    def.curve = HapticCurve::Punch;
    def.priority = priority;
    def.peak.leftMotor = 0.0f;
    def.peak.rightMotor = 0.15f;
    def.peak.leftTrigger = 0.0f;
    def.peak.rightTrigger = 0.1f;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeEngineLoop(float intensity)
{
    HapticEffectDefinition def;
    def.id = "EngineHum";
    def.durationSeconds = 1.0f;
    def.curve = HapticCurve::Sustain;
    def.priority = HapticPriority::Low;
    def.peak.leftMotor = intensity * 0.18f;
    def.peak.rightMotor = intensity * 0.12f;
    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeHeartbeatLub(float intensity, HeartbeatVariant variant)
{
    const float i = intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity);

    HapticEffectDefinition def;
    def.durationSeconds = 0.08f;   
    def.attackSeconds = 0.005f; 
    def.curve = HapticCurve::Punch;
    def.priority = HapticPriority::High;

    if (variant == HeartbeatVariant::Health)
    {
        def.id = "HeartbeatLub_Health";

        def.peak.leftMotor = i;    
        def.peak.rightMotor = i * 0.75f;

        if (i > 0.55f)
        {
            const float triggerAmount = (i - 0.55f) / 0.45f; 
            def.peak.leftTrigger = triggerAmount * 0.50f;
            def.peak.rightTrigger = triggerAmount * 0.50f;
        }
    }
    else 
    {
        def.id = "HeartbeatLub_Separation";

        def.peak.rightMotor = i;      
        def.peak.leftMotor = i * 0.60f;

        def.peak.leftTrigger = 0.0f;
        def.peak.rightTrigger = 0.0f;
    }

    return def;
}

HapticEffectDefinition HapticEffectDefinition::makeHeartbeatDub(float intensity, HeartbeatVariant variant)
{
    const float i = intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity);

    const float dubScale = (variant == HeartbeatVariant::Health) ? 0.65f : 0.55f;
    const float di = i * dubScale;

    HapticEffectDefinition def;
    def.durationSeconds = 0.08f;
    def.attackSeconds = 0.005f;
    def.curve = HapticCurve::Punch;
    def.priority = HapticPriority::High;

    if (variant == HeartbeatVariant::Health)
    {
        def.id = "HeartbeatDub_Health";

        def.peak.leftMotor = di;
        def.peak.rightMotor = di * 0.75f;

        if (i > 0.70f)
        {
            const float triggerAmount = (i - 0.70f) / 0.30f;
            def.peak.leftTrigger = triggerAmount * 0.30f; 
            def.peak.rightTrigger = triggerAmount * 0.30f;
        }
    }
    else
    {
        def.id = "HeartbeatDub_Separation";

        def.peak.rightMotor = di;
        def.peak.leftMotor = di * 0.60f;
        def.peak.leftTrigger = 0.0f;
        def.peak.rightTrigger = 0.0f;
    }

    return def;
}