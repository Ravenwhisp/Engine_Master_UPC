#pragma once

#include "CharacterBase.h"

class DeathSound;
class PlayerMovement;
class DeathConfig;

class DeathCharacter : public CharacterBase
{
    DECLARE_SCRIPT(DeathCharacter)

public:
    explicit DeathCharacter(GameObject* owner);

    FieldList getExposedFields() const override;

    void Start()  override;
    void Update() override;

    DeathSound* getSound() const { return m_sound; }

    float getComboWindowR2() const;
    float getComboWindowMaxCharge() const;
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

private:
    void tickCombo(float dt);

private:
    int   m_comboStep           = 0;
    int   m_consecutiveR2Count  = 0;
    float m_comboTimer          = 0.0f;
    float m_comboCooldownTimer  = 0.0f;
    float m_activeComboWindow   = 0.0f;

    DeathSound*     m_sound     = nullptr;
    PlayerMovement* m_movement  = nullptr;
    AssetRef<DeathConfig> m_config;
};
