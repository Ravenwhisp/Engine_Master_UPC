#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <vector>

enum class HapticCurve : uint8_t
{
    Linear,    
    Exponential,  
    Sustain,    
    Punch,        
};

enum class HapticPriority : uint8_t
{
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3, 
};

struct HapticEffect
{
    float leftMotor = 0.0f;  
    float rightMotor = 0.0f;  
    float leftTrigger = 0.0f; 
    float rightTrigger = 0.0f;

    float durationSeconds = 0.2f;  
    float delaySeconds = 0.0f;  

    HapticCurve    curve = HapticCurve::Linear;
    HapticPriority priority = HapticPriority::Normal;

    static HapticEffect makeImpact(float intensity = 1.0f,
        float durationSeconds = 0.12f,
        HapticPriority priority = HapticPriority::Normal)
    {
        HapticEffect e;
        e.leftMotor = intensity;
        e.rightMotor = intensity * 0.6f;
        e.leftTrigger = intensity * 0.4f;
        e.rightTrigger = intensity * 0.4f;
        e.durationSeconds = durationSeconds;
        e.curve = HapticCurve::Punch;
        e.priority = priority;
        return e;
    }

    static HapticEffect makeContinuous(float leftIntensity = 0.3f,
        float rightIntensity = 0.15f,
        float durationSeconds = 1.0f,
        HapticPriority priority = HapticPriority::Low)
    {
        HapticEffect e;
        e.leftMotor = leftIntensity;
        e.rightMotor = rightIntensity;
        e.durationSeconds = durationSeconds;
        e.curve = HapticCurve::Sustain;
        e.priority = priority;
        return e;
    }

    static HapticEffect makeExplosion(float peakIntensity = 1.0f,
        float durationSeconds = 0.6f,
        HapticPriority priority = HapticPriority::High)
    {
        HapticEffect e;
        e.leftMotor = peakIntensity;
        e.rightMotor = peakIntensity * 0.5f;
        e.leftTrigger = peakIntensity * 0.3f;
        e.rightTrigger = peakIntensity * 0.3f;
        e.durationSeconds = durationSeconds;
        e.curve = HapticCurve::Exponential;
        e.priority = priority;
        return e;
    }
};

#include "GamePad.h"

class ModuleHaptics : public Module
{
public:
    static constexpr int MAX_PLAYERS = 4;

    ModuleHaptics();
    ~ModuleHaptics() override;

    bool init()    override;
    void update()  override;
    bool cleanUp() override;

    uint32_t submitEffect(const HapticEffect& effect, int player = 0);

    void cancelEffect(uint32_t handle, int player = 0);

    void cancelAll(int player = 0);

    void silenceAll();

    bool isPlaying(int player = 0) const;
    void logConnectedControllers();

private:
    struct ActiveEffect
    {
        HapticEffect effect;
        float        elapsed = 0.0f;   
        float        delay = 0.0f;   
        uint32_t     handle = 0;
        bool         alive = true;
    };

    struct PlayerState
    {
        std::vector<ActiveEffect> activeEffects;
         
        float leftMotor = 0.0f;
        float rightMotor = 0.0f;
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;
    };

    std::array<PlayerState, MAX_PLAYERS> m_players;

    uint32_t m_nextHandle = 1;  
     
    float evaluateCurve(const ActiveEffect& ae) const;
    void tickPlayer(int playerIndex, float dt);
    void applyToHardware(int playerIndex);
    static float clamp01(float v);
};