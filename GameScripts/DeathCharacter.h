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

public:
    float m_comboWindow            = 1.0f;
    float m_comboWindowR2          = 2.0f;
    float m_comboWindowMaxCharge   = 3.0f;
    float m_comboCooldown          = 1.0f;

private:
    void tickCombo(float dt);

private:
    int   m_comboStep           = 0;
    int   m_consecutiveR2Count  = 0;
    float m_comboTimer          = 0.0f;
    float m_comboCooldownTimer  = 0.0f;
    float m_activeComboWindow   = 0.0f;

};
