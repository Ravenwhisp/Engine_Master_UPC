#pragma once

#include "ScriptAPI.h"
#include "CharacterUI.h"

class CharacterBase;

class AbilityBase : public Script
{

    DECLARE_SCRIPT(AbilityBase)

public:
    explicit AbilityBase(GameObject* owner);

    void Start() override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

    bool isEnabled() const { return m_isEnabled; }
    void setEnabled(bool enabled) { m_isEnabled = enabled; }

    void tryAbility();

    // Reduces the current cooldown timer by (fraction * baseCooldown).
    // Clamps to 0. Hides the CD UI if it reaches 0.
    void reduceCooldown(float fraction);

protected:
	virtual void startAbility() {}

    bool canStartAbility() const;
    virtual bool canStartSpecificAbility() const { return true; }

    virtual float getCooldown() const { return m_cooldown; }
    void updateCooldown(float dt);
	void updateAttackWindow(float dt);
	void startCooldown();
    bool isCooldownReady() const { return m_cooldownTimer <= 0.0f; }

    void setAbilityLocked(bool locked); //innecesario
    int getPlayerIndex() const; //innecesario

    virtual void beginAttackWindow(float lockDuration);
    virtual void finishAttackWindow();

    void beginAttackPresentation();

    virtual void onAttackWindowUpdate() {}
    virtual void onAttackWindowFinished() {}

    Vector3 computeCameraRelativeAimDirection(float deadzoneSq = 0.0001f) const;
	Vector3 getFallbackFacingDirection() const;

    virtual void updateUI();

protected:
    CharacterBase* m_character = nullptr;
    CharacterUI* m_characterUI = nullptr;
    int m_uiSlot = static_cast<int>(AbilityUISlot::BasicAttack);

    float m_cooldown = 0.0;
    float m_cooldownTimer = 0.0f;

    float m_attackStateTimer = 0.0f;

    bool m_isEnabled = true; //esto nunca cambia?
};