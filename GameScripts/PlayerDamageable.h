#pragma once

#include "Damageable.h"

class PlayerAnimationController;
class HeartbeatHaptic;
class DeathSound;
class LyrielSound;

class PlayerDamageable : public Damageable
{
    DECLARE_SCRIPT(PlayerDamageable)

public:
    explicit PlayerDamageable(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

    float m_heartbeatThreshold = 0.5f;

protected:
    void onDamaged(float amount) override;
    void onHpDepleted() override;
    void onDeath() override;
    void onRevive() override;

private:
    PlayerAnimationController* m_playerAnimationController = nullptr;
    HeartbeatHaptic* m_haptic = nullptr;
    DeathSound*  m_deathSound  = nullptr;
    LyrielSound* m_lyrielSound = nullptr;
    PlayerRenderBufferComponent* m_playerRenderBuffer = nullptr;

    ComponentRef<Transform> m_renderer;

    bool  m_damageHighlightActive = false;
    float m_damageHighlightTimer = 0.0f;
    float m_damageHighlightSpeed = 1.0f;

    void playHurtSfx();
    void playHurtVfx();
};