#pragma once

#include "DeathAbilityBase.h"

// Death's heavy attack — R2 (RT) with two modes:
//
//   Quick press  (release before m_minChargeTime):
//     Deals m_chargedAttackDamage in the attack arc. Can be used mid-combo
//     (up to 2 consecutive R2s, then R1 required). Combo window: m_comboWindowR2.
//
//   Charged press (hold >= m_minChargeTime, step 0 only — combo starter):
//     Right stick controls aim direction while charging.
//     Auto-fires when charge reaches m_maxChargeTime (no need to release).
//     Damage scales 1x–2x based on charge ratio. Combo window: m_comboWindowR2.
//     At max charge (auto-fire): combo window is m_comboWindowMaxCharge.
//     A charged shot is NEVER used mid-combo — step must be 0.
//
// Movement locks during the entire duration: while charging AND during the
// post-fire attack window, matching DeathBasicAttack's combo lock pattern.
class DeathChargedAttack : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathChargedAttack)

public:
    explicit DeathChargedAttack(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_minChargeTime         = 0.5f;
    float m_attackLockDuration    = 0.4f;
    float m_finalHitLockDuration  = 0.8f;
    float m_chargedArcRange       = 3.5f;
    float m_chargedArcAngle       = 150.0f;

protected:
    void onAttackWindowUpdate()   override;
    void onAttackWindowFinished() override;

private:
    void startCharging();
    void fireAttack();
    void updateAimDirection();
    void snapFaceAimDirection();
    void releaseComboMoveLock();

    float   m_chargeTime             = 0.0f;
    bool    m_isCharging             = false;
    bool    m_movementLockedForCombo = false;
    Vector3 m_aimDirection           = { 0.0f, 0.0f, 0.0f };
};
