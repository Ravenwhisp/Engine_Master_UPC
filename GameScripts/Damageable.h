#pragma once

#include "ScriptAPI.h"
#include "UISlider.h"
#include "UISheet.h"

struct HitContext
{
    float damage = 0.0f;
    // Continuous = damage applied every frame by a sustained source (Bound
    // separation, DoTs, environmental). Lets reaction feedback (hurt SFX, flinch)
    // treat it differently from discrete hits instead of firing every frame.
    bool  continuous = false;
};

class Damageable : public Script
{
    DECLARE_SCRIPT(Damageable)

public:
    explicit Damageable(GameObject* owner);

    void Start()     override;
	void Update()	 override;
    void drawGizmo() override;
    ScriptFieldList getExposedFields() const override;

    virtual void takeDamage(float amount);
    virtual void takeDamage(const HitContext& ctx);
    void heal(float amount);
    virtual void kill();
    void revive(float hp = -1.0f);

    float getCurrentHp() const { return m_currentHp; }
    float getMaxHp() const { return m_maxHp; }
    float getHpPercent() const;
    bool isDead() const { return m_isDead; }

    void setInvulnerable(bool invulnerable) { m_invulnerable = invulnerable; }
    bool isInvulnerable() const { return m_invulnerable; }

    // True if the most recent damage came from a continuous source. Valid inside
    // onDamaged() and right after a takeDamage() call.
    bool isLastDamageContinuous() const { return m_damageIsContinuous; }

    bool m_isGaugeExecution = false;

protected:
    virtual void onDamaged(float amount);
    virtual void onHealed(float amount);
    virtual void onHpDepleted();
    virtual void onDeath();
    virtual void onRevive();

private:
    void applyDamage(float amount, bool continuous);
    void clampHp();
    void setupUI();
    void updateUI();

public:
    float m_maxHp = 100.0f;
	ScriptComponentRef<UISlider> m_healthBar;
    ScriptComponentRef<UISlider> m_healthBar2;
    ScriptComponentRef<UISlider> m_healthGlow;
    float m_uiWaitTime = 0.6f;
    float m_uiUpdateTime = 1.0f;

protected:
    float m_currentHp   = 100.0f;
    bool  m_invulnerable = false;
    bool  m_isDead       = false;
    bool  m_damageIsContinuous = false;

	UISlider* m_healthBarSlider = nullptr;
	UISlider* m_healthBar2Slider = nullptr;
    UISlider* m_healthGlowSlider = nullptr;
    UISheet* m_healthGlowSheet = nullptr;
	float m_uiTimer = 0.0f;
	float m_currentDisplayedHp = 100.0f;
	float m_previousHp = 100.0f;
    float m_uiStartPercent;
    float m_uiTargetPercent;
};