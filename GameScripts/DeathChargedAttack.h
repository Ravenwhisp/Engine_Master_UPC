#pragma once

#include "DeathAbilityBase.h"

class DeathUI;

class DeathChargedAttack : public DeathAbilityBase
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
    void dealDamageInArc(float damage, float range, float angle, bool isChargedShot, bool isMaxCharge) const;
    void updateAimDirection();
    void snapFaceAimDirection();

    void updateUI() override;

private:
    DeathUI* m_deathUI = nullptr;

    float   m_chargeTime = 0.0f;
    bool    m_isCharging = false;
    Vector3 m_aimDirection = { 0.0f, 0.0f, 0.0f };
};
