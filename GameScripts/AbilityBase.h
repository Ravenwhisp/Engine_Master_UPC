#pragma once

#include "ScriptAPI.h"

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

protected:
    CharacterBase* m_character = nullptr;

	virtual void updateUI();
    float m_cooldown = 0.0f;
    float m_cooldownTimer = 0.0f;
    ScriptComponentRef<Transform> m_cdUI;
    GameObject* m_cdGO = nullptr;
    ScriptComponentRef<UISlider> m_cdBar;
	UISlider* m_cdBarSlider = nullptr;

    float m_attackStateTimer = 0.0f;

    bool m_isEnabled = true; //esto nunca cambia?
};