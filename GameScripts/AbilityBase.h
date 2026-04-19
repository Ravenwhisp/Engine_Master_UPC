#pragma once

#include "ScriptAPI.h"

class CharacterBase;

class AbilityBase : public Script
{
public:
    explicit AbilityBase(GameObject* owner);

    void Start() override;
    void Update() override;

    bool isEnabled() const { return m_isEnabled; }
    void setEnabled(bool enabled) { m_isEnabled = enabled; }

protected:
    virtual bool canStartAbility() const;

    void updateCooldown();
    bool isCooldownReady() const { return m_cooldownTimer <= 0.0f; }

    void setAbilityLocked(bool locked);
    int getPlayerIndex() const;

    CharacterBase* findCharacterScript(GameObject* owner) const;

protected:
    CharacterBase* m_character = nullptr;

    float m_cooldown = 0.0f;
    float m_cooldownTimer = 0.0f;

    bool m_isEnabled = true;
};