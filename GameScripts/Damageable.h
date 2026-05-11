#pragma once

#include "ScriptAPI.h"
#include "UISlider.h"

class Damageable : public Script
{
    DECLARE_SCRIPT(Damageable)

public:
    explicit Damageable(GameObject* owner);

    void Start()     override;
    void drawGizmo() override;
    ScriptFieldList getExposedFields() const override;

    void takeDamage(float amount);
    void heal(float amount);
    void kill();
    void revive(float hp = -1.0f);

    float getCurrentHp() const { return m_currentHp; }
    float getMaxHp() const { return m_maxHp; }
    float getHpPercent() const;
    bool isDead() const { return m_isDead; }

    void setInvulnerable(bool invulnerable) { m_invulnerable = invulnerable; }
    bool isInvulnerable() const { return m_invulnerable; }

protected:
    virtual void onDamaged(float amount);
    virtual void onHealed(float amount);
    virtual void onHpDepleted();
    virtual void onDeath();
    virtual void onRevive();

private:
    void clampHp();
    void updateHealthBar();

public:
    float m_maxHp = 100.0f;
	ScriptComponentRef<UISlider> m_healthBar;

private:
    float m_currentHp   = 100.0f;
    bool  m_invulnerable = false;
    bool  m_isDead       = false;

};