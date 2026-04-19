#include "pch.h"
#include "LyrielDash.h"

static const ScriptFieldInfo LyrielDashFields[] =
{
    { "Dash Duration", ScriptFieldType::Float, offsetof(LyrielDash, m_dashDurationLyriel), { 0.0f, 1.0f, 0.01f } },
    { "Dash Distance", ScriptFieldType::Float, offsetof(LyrielDash, m_dashDistanceLyriel), { 0.0f, 10.0f, 0.1f } },
    { "Dash Cooldown", ScriptFieldType::Float, offsetof(LyrielDash, m_dashCooldown), { 0.0f, 5.0f, 0.01f } },
    { "Charge Recharge Time", ScriptFieldType::Float, offsetof(LyrielDash, m_chargeRechargeTime), { 0.1f, 10.0f, 0.1f } }
};

IMPLEMENT_SCRIPT_FIELDS(LyrielDash, LyrielDashFields)

LyrielDash::LyrielDash(GameObject* owner)
    : AbilityDash(owner)
{
}

void LyrielDash::Start()
{
    m_dashDuration = m_dashDurationLyriel;
    m_dashDistance = m_dashDistanceLyriel;
    m_cooldown = m_dashCooldown;

    AbilityDash::Start();
}

void LyrielDash::recoverCharge()
{
    if (m_charges < MAX_DASH_CHARGES)
    {
        ++m_charges;

        if (m_charges == MAX_DASH_CHARGES)
        {
            m_chargeRecoveryTimer = 0.0f;
        }
    }
}

bool LyrielDash::canDash() const
{
    return m_charges > 0;
}

void LyrielDash::onDashStarted()
{
    --m_charges;
}

void LyrielDash::onDashUpdate(float dt)
{
    if (m_charges >= MAX_DASH_CHARGES)
    {
        return;
    }

    m_chargeRecoveryTimer += dt;

    while (m_chargeRecoveryTimer >= m_chargeRechargeTime && m_charges < MAX_DASH_CHARGES)
    {
        ++m_charges;
        m_chargeRecoveryTimer -= m_chargeRechargeTime;
    }

    if (m_charges >= MAX_DASH_CHARGES)
    {
        m_charges = MAX_DASH_CHARGES;
        m_chargeRecoveryTimer = 0.0f;
    }
}

IMPLEMENT_SCRIPT(LyrielDash)