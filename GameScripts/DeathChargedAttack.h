#pragma once

#include "ChargedAttackBase.h"

class DeathUI;
class DeathParticles;
class DeathConfig;
class DeathCharacter;

class DeathChargedAttack : public ChargedAttackBase
{
    DECLARE_SCRIPT(DeathChargedAttack)

public:
    explicit DeathChargedAttack(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

protected:
    void startAbility() override;

    bool canStartSpecificAbility() const override;

    void onAttackWindowUpdate()     override;
    void onAttackWindowFinished()   override;

    float getCooldown() const override;

private:
    void startCharging();
    void fireAttack();
    void dealDamageInCircle(float damage, float radius, bool isChargedShot, bool isMaxCharge) const;

    void updateUI() override;

private:
    DeathCharacter* m_deathCharacter = nullptr;
    DeathConfig* m_config = nullptr;
    DeathUI* m_deathUI = nullptr;
    DeathParticles* m_particles = nullptr;

    float   m_chargeTime = 0.0f;
    bool    m_isCharging = false;
};
