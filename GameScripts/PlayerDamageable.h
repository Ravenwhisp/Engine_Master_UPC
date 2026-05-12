#pragma once

#include "Damageable.h"
#include "HapticEffectDefinition.h"

class PlayerAnimationController;

class PlayerDamageable : public Damageable
{
    DECLARE_SCRIPT(PlayerDamageable)

public:
    explicit PlayerDamageable(GameObject* owner);

    void Start()  override;
    void Update() override;

protected:
    void onDamaged(float amount) override;
    void onHpDepleted()          override;
    void onDeath()               override;
    void onRevive()              override;

public:
    float m_heartbeatThreshold = 0.5f;  // heartbeat begins below this HP fraction

    // Test controls (inspector-tweakable)
    int   m_testPlayerIndex = 0;
    float m_testDamageAmount = 10.0f;
    float m_testHealAmount = 10.0f;

private:
    void fireLub();

    PlayerAnimationController* m_playerAnimationController = nullptr;

    float m_dubTimer = -1.0f;  // counts down to firing the dub;  -1 = inactive
    float m_lubTimer = -1.0f;  // counts down to firing next lub (diastole wait)
    float m_dubScale = 0.0f;  // danger scale captured at lub-fire time
    bool  m_dyingBeat = false;  // true when the current cycle is the final death beat
};