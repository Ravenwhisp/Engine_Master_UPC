#pragma once

#include "ScriptAPI.h"
#include "CharacterUI.h"

class CharacterBase;
class AnimationComponent;
class CharacterAnimations;

class AbilityBase : public Script
{

    DECLARE_SCRIPT(AbilityBase)

public:
    explicit AbilityBase(GameObject* owner);

    void Start() override;
    void Update() override;
    FieldList getExposedFields() const override;

    bool isEnabled() const { return m_isEnabled; }
    void setEnabled(bool enabled) { m_isEnabled = enabled; }

    void tryAbility();

    // Reduces the current cooldown timer by (fraction * baseCooldown).
    // Clamps to 0. Hides the CD UI if it reaches 0.
    void reduceCooldown(float fraction);

    int getSuccessfulUse() const { return m_successfulUseCount; }

protected:
	virtual void startAbility() {}

    void notifyAbilitySuccessfullyStarted();

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
    virtual void onHitFrame() {}

    virtual int getAttackVariant() const { return 0; }

    bool isAttackWindowActive() const { return m_attackWindowActive; }
    bool usesAnimHitTiming() const;
    void resolveCurrentAttackAnim();

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

    int m_successfulUseCount = 0;

    bool m_isEnabled = true; //esto nunca cambia?

    AnimationComponent* m_animComp = nullptr;
    CharacterAnimations* m_attackAnims = nullptr;

    bool  m_attackWindowActive = false;
    bool  m_hitFired = false;
    bool  m_sawOurClip = false;
    float m_attackWindowElapsed = 0.0f;

    std::string m_curAnimState = "";
    std::string m_curRecoveryState = "";
    float m_curAnimSpeed = 1.0f;
    float m_curAnimBlend = 0.15f;
    float m_curHitPct = 0.30f;
    float m_curRecoverPct = 0.90f;
};