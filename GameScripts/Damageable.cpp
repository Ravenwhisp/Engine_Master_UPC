#include "pch.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(Damageable,
    SERIALIZED_FLOAT(m_maxHp, "Max HP", 0.0f, 999999.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_healthBar, "Health Slider", ComponentType::UISLIDER),
    SERIALIZED_COMPONENT_REF(m_healthBar2, "Health Slider 2", ComponentType::UISLIDER),
	SERIALIZED_FLOAT(m_uiUpdateTime, "UI Update Time (Slider 2)", 0.0f, 999999.0f, 0.1f)
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

    m_healthBarSlider = m_healthBar.getReferencedComponent();
    m_healthBar2Slider = m_healthBar2.getReferencedComponent();
	SliderAPI::setFillAmount(m_healthBarSlider, getHpPercent());
    SliderAPI::setFillAmount(m_healthBar2Slider, getHpPercent());

    updateUI();
}

void Damageable::Update()
{
    updateUI();
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
    }

    clampHp();

    if (m_currentHp <= 0.0f && m_maxHp > 0.0f)
    {
        m_currentHp = m_maxHp;
    }

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

void Damageable::updateUI()
{
    if (m_healthBarSlider)
    {
        SliderAPI::setFillAmount(m_healthBarSlider, getHpPercent());
    }

    if (m_healthBar2Slider)
    {
        if (m_currentDisplayedHp != m_currentHp)
        {
            m_uiStartPercent = SliderAPI::getFillAmount(m_healthBar2Slider);
            m_uiTargetPercent = getHpPercent();
            m_uiTimer = m_uiUpdateTime;
		}

        if (m_uiTimer > 0.0f)
        {
            m_uiTimer -= Time::getDeltaTime();

            float t = 1.0f - (m_uiTimer / m_uiUpdateTime);
            float eased = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);
            float value = MathAPI::lerp(m_uiStartPercent, m_uiTargetPercent, eased);

            SliderAPI::setFillAmount(m_healthBar2Slider, value);

            if (m_uiTimer <= 0.0f)
            {
                m_currentDisplayedHp = m_currentHp;
            }
        }
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