#include "pch.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(Damageable,
    SERIALIZED_FLOAT(m_maxHp, "Max HP", 0.0f, 999999.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_healthBar, "Health Slider", ComponentType::UISLIDER)
)

Damageable::Damageable(GameObject* owner)
    : Script(owner)
{
}

void Damageable::drawGizmo()
{
}

void Damageable::Start()
{
    m_currentHp = m_maxHp;
    clampHp();
    m_isDead = (m_currentHp <= 0.0f);

    updateHealthBar();
}

void Damageable::takeDamage(float amount)
{
    if (m_isDead)
    {
        return;
    }

    if (m_invulnerable)
    {
        return;
    }

    if (amount <= 0.0f)
    {
        return;
    }

    m_currentHp -= amount;
    clampHp();

    onDamaged(amount);

    updateHealthBar();

    if (m_currentHp <= 0.0f)
    {
        onHpDepleted();
    }
}

void Damageable::heal(float amount)
{
    if (m_isDead)
    {
        return;
    }

    if (amount <= 0.0f)
    {
        return;
    }

    m_currentHp += amount;
    clampHp();

    onHealed(amount);

    updateHealthBar();
}

void Damageable::kill()
{
    if (m_isDead)
    {
        return;
    }

    m_currentHp = 0.0f;
    m_isDead = true;

    updateHealthBar();

    onDeath();
}

void Damageable::revive(float hp)
{
    m_isDead = false;

    if (hp < 0.0f)
    {
        m_currentHp = m_maxHp;
    }
    else
    {
        m_currentHp = hp;
    }

    clampHp();

    if (m_currentHp <= 0.0f && m_maxHp > 0.0f)
    {
        m_currentHp = m_maxHp;
    }

    updateHealthBar();

    onRevive();
}

float Damageable::getHpPercent() const
{
    if (m_maxHp <= 0.0f)
    {
        return 0.0f;
    }

    return m_currentHp / m_maxHp;
}

void Damageable::clampHp()
{
    if (m_maxHp < 0.0f)
    {
        m_maxHp = 0.0f;
    }

    if (m_currentHp < 0.0f)
    {
        m_currentHp = 0.0f;
    }

    if (m_currentHp > m_maxHp)
    {
        m_currentHp = m_maxHp;
    }
}

void Damageable::updateHealthBar()
{
    UISlider* healthBar = m_healthBar.getReferencedComponent();
    if (healthBar)
    {
        SliderAPI::setFillAmount(healthBar, getHpPercent());
    }
}

void Damageable::onDamaged(float amount)
{
    Debug::log("%s took %.2f damage. HP: %.2f / %.2f", GameObjectAPI::getName(m_owner), amount, m_currentHp, m_maxHp);
}

void Damageable::onHealed(float amount)
{
    Debug::log("%s healed %.2f HP. HP: %.2f / %.2f", GameObjectAPI::getName(m_owner), amount, m_currentHp, m_maxHp);
}

void Damageable::onHpDepleted()
{
    kill();
}

void Damageable::onDeath()
{
    Debug::log("%s died.", GameObjectAPI::getName(m_owner));
}

void Damageable::onRevive()
{
    Debug::log("%s revived. HP: %.2f / %.2f", GameObjectAPI::getName(m_owner), m_currentHp, m_maxHp);
}

IMPLEMENT_SCRIPT(Damageable)