#pragma once

#include "CharacterBase.h"

class DeathCharacter : public CharacterBase
{
    DECLARE_SCRIPT(DeathCharacter)

public:
    explicit DeathCharacter(GameObject* owner);

    void Start()  override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    int   getComboStep()       const { return m_comboStep; }
    bool  canUseR2InCombo()    const { return m_consecutiveR2Count < 2; }
    bool  isInComboCooldown()  const { return m_comboCooldownTimer > 0.0f; }
    bool  wasLastHitR2()       const { return m_consecutiveR2Count > 0; }

    float getComboFillRatio()  const
    {
        if (m_comboStep == 0 || m_activeComboWindow <= 0.0f) return 0.0f;
        const float r = 1.0f - (m_comboTimer / m_activeComboWindow);
        return r < 0.0f ? 0.0f : r;
    }

    void advanceCombo(bool isR2, float comboWindowOverride = -1.0f);
    void resetCombo();
    void dealDamageBasicAttack(float damage, GameObject* target) const;
    void dealDamageInArc(float damage) const;
    void dealDamageInArc(float damage, float range, float angle) const;

public:
    float m_basicAttackDamage    = 20.0f;
    float m_basicAttackRange     = 1.5f;
    float m_basicAttackHitAngle  = 50.0f;
    float m_chargedAttackDamage  = 40.0f;
    float m_dashDistance         = 5.0f;
    float m_tauntDuration        = 2.0f;
    float m_arcRange             = 2.5f;
    float m_arcAngle             = 120.0f;
    float m_maxChargeTime          = 2.0f;
    float m_comboWindow            = 1.0f;
    float m_comboWindowR2          = 2.0f;
    float m_comboWindowMaxCharge   = 3.0f;
    float m_comboCooldown          = 1.0f;

private:
    int   m_comboStep           = 0;
    int   m_consecutiveR2Count  = 0;
    float m_comboTimer          = 0.0f;
    float m_comboCooldownTimer  = 0.0f;
    float m_activeComboWindow   = 0.0f;

    void tickCombo(float dt);
};
