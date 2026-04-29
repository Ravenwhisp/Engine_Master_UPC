#pragma once

#include "Module.h"
#include "HapticEffectDefinition.h"
#include "HapticEffectLibrary.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class ModuleHaptics : public Module
{
public:
    static constexpr int MAX_PLAYERS = 4;
    static constexpr int MAX_INSTANCES = 16;  

    ModuleHaptics();
    ~ModuleHaptics() override;
     
    bool init()    override;
    void update()  override;
    bool cleanUp() override;
 
    uint32_t playEffect(const std::string& effectId, int player = 0);
    uint32_t playAtScale(const std::string& effectId, float scale, int player = 0);

    uint32_t submitAnonymous(const HapticEffectDefinition& def, float scale = 1.0f, int player = 0);

    void cancelEffect(uint32_t handle, int player = 0);
    void cancelAll(int player = 0);

    void silenceAll();

    bool isPlaying(int player = 0) const;

    void getMixedOutput(int player, float& outLeft, float& outRight, float& outLeftTrigger, float& outRightTrigger) const;

    uint32_t submitEffect(const HapticEffectDefinition& def, int player = 0);

    void logActiveEffects(int player = 0) const;
    void logConnectedControllers() const;

private:
    struct HapticInstance
    {
        const HapticEffectDefinition* def = nullptr;
        float elapsed = 0.0f;
        float delay = 0.0f;
        float scale = 1.0f;  
        uint32_t handle = 0;
        bool alive = true;

        HapticEffectDefinition anonDef;
    };

    struct PlayerState
    {
        std::vector<HapticInstance> instances; 

        float leftMotor = 0.0f;
        float rightMotor = 0.0f;
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;
    };

    uint32_t submitInternal(const HapticEffectDefinition* def, float scale, int player, const HapticEffectDefinition* anonDefOwner = nullptr);

    void tickPlayer(int playerIndex, float dt);

    void applyToHardware(int playerIndex) const;

    float evaluateEnvelope(const HapticInstance& inst) const;

    static float clamp01(float v);

    std::array<PlayerState, MAX_PLAYERS> m_players;

    uint32_t m_nextHandle = 1; 
};