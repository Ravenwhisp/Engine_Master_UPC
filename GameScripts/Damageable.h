#pragma once

#include "ScriptAPI.h"
#include "UISlider.h"

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
    FieldList getExposedFields() const override;

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

    void setupUI();
    void updateUI();
    virtual void onHealthUIChanged(float previousHpPercent, float currentHpPercent) {}

private:
    void applyDamage(float amount, bool continuous);
    void clampHp();

protected:
    float m_maxHp = 100.0f;

    ComponentRef<UISlider> m_healthBar;
    ComponentRef<UISlider> m_healthBar2;

    float m_uiWaitTime = 0.6f;
    float m_uiUpdateTime = 1.0f;

    float m_currentHp   = 100.0f;
    bool  m_invulnerable = false;
    bool  m_isDead       = false;
    bool  m_damageIsContinuous = false;

	UISlider* m_healthBarSlider = nullptr;
	UISlider* m_healthBar2Slider = nullptr;

private:
    float m_uiTimer = 0.0f;
    float m_previousHp = 100.0f;

    float m_uiStartPercent = 1.0f;
    float m_uiTargetPercent = 1.0f;
};