#pragma once

#include "Damageable.h"

class PlayerAnimationController;
class HeartbeatHaptic;

class PlayerDamageable : public Damageable
{
    DECLARE_SCRIPT(PlayerDamageable)

public:
    explicit PlayerDamageable(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    float m_heartbeatThreshold = 0.5f;

protected:
    void onDamaged(float amount) override;
    void onHpDepleted() override;
    void onDeath() override;
    void onRevive() override;

private:
    PlayerAnimationController* m_playerAnimationController = nullptr;
    HeartbeatHaptic* m_haptic = nullptr;
};