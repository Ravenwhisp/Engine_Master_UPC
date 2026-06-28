#include "pch.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(Damageable,
    SERIALIZED_FLOAT(m_maxHp, "Max HP", 0.0f, 999999.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_healthBar, "Health Slider", ComponentType::UISLIDER),
    SERIALIZED_COMPONENT_REF(m_healthBar2, "Health Slider 2", ComponentType::UISLIDER),
	SERIALIZED_COMPONENT_REF(m_healthGlow, "Health Glow", ComponentType::UISLIDER),
	SERIALIZED_FLOAT(m_uiUpdateTime, "UI Update Time (Slider 2)", 0.0f, 999999.0f, 0.1f),
	SERIALIZED_FLOAT(m_uiWaitTime, "UI Wait Time (Slider 2)", 0.0f, 999999.0f, 0.0f)
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

    setupUI();
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

void Damageable::takeDamage(const HitContext& ctx)
{
    takeDamage(ctx.damage);
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

void Damageable::setupUI()
{
    m_healthBarSlider = m_healthBar.getReferencedComponent();
    m_healthBar2Slider = m_healthBar2.getReferencedComponent();
	m_healthGlowSlider = m_healthGlow.getReferencedComponent();

    m_currentDisplayedHp = m_currentHp;
	m_healthGlowSheet = (m_healthGlowSlider) ? static_cast<UISheet*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(m_healthGlowSlider), ComponentType::UISHEET)) : nullptr;
	if (m_healthBarSlider)
    {
        SliderAPI::setFillAmount(m_healthBarSlider, getHpPercent());
    }
    if (m_healthBar2Slider)
    {
        SliderAPI::setFillAmount(m_healthBar2Slider, getHpPercent());
    }
    if (m_healthGlowSlider)
    {
        SliderAPI::setFillAmount(m_healthGlowSlider, getHpPercent());
	}
}

void Damageable::updateUI()
{
    if (m_healthBarSlider)
    {
        SliderAPI::setFillAmount(m_healthBarSlider, getHpPercent());
    }

    if (m_previousHp != m_currentHp)
    {
        if (m_healthGlowSlider && m_healthGlowSheet)
        {
			SliderAPI::setFillAmountVec(m_healthGlowSlider, Vector2(getHpPercent(), SliderAPI::getFillAmount(m_healthBar2Slider)));
            UISheetAPI::play(m_healthGlowSheet);
		}

        if (m_healthBar2Slider)
        {
            m_uiStartPercent = SliderAPI::getFillAmount(m_healthBar2Slider);
            m_uiTargetPercent = getHpPercent();
            m_uiTimer = m_uiUpdateTime + m_uiWaitTime;
        }

		m_previousHp = m_currentHp;
	}

    if (m_healthBar2Slider)
    {
        if (m_uiTimer > 0.0f)
        {
            m_uiTimer -= Time::getDeltaTime();

            if (m_uiTimer > m_uiUpdateTime)
            {
                return;
            }

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