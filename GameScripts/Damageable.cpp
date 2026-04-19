#include "pch.h"
#include "Damageable.h"

static const ScriptFieldInfo damageableFields[] =
{
    { "Max HP", ScriptFieldType::Float, offsetof(Damageable, m_maxHp), { 0.0f, 999999.0f, 1.0f } },
    { "Health Slider", ScriptFieldType::ComponentRef, offsetof(Damageable, m_healthBar), {}, {},{ ComponentType::UISLIDER } },
};

IMPLEMENT_SCRIPT_FIELDS(Damageable, damageableFields)

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

	UISlider* healthBar = m_healthBar.getReferencedComponent();
    if (healthBar)
    {
		SliderAPI::setFillAmount(healthBar, getHpPercent());
	}

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
}

void Damageable::kill()
{
    if (m_isDead)
    {
        return;
    }

    m_currentHp = 0.0f;
    m_isDead = true;

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
        clampHp();

        if (m_currentHp <= 0.0f && m_maxHp > 0.0f)
        {
            m_currentHp = m_maxHp;
        }
    }

    clampHp();
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