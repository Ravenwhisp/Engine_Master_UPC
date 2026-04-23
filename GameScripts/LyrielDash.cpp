#include "pch.h"
#include "LyrielDash.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielDash, AbilityDash,
    SERIALIZED_FLOAT(m_chargeRechargeTime, "Charge Recharge Time", 0.1f, 10.0f, 0.1f),
    SERIALIZED_INT(m_maxCharges, "Max charges")
)

LyrielDash::LyrielDash(GameObject* owner)
    : AbilityDash(owner)
{
}

void LyrielDash::Start()
{
    AbilityDash::Start();

    m_currentCharges = m_maxCharges;
}

void LyrielDash::recoverCharge()
{
    if (m_currentCharges < m_maxCharges)
    {
        ++m_currentCharges;

        if (m_currentCharges == m_maxCharges)
        {
            m_chargeRecoveryTimer = 0.0f;
        }
    }
}

bool LyrielDash::canDash() const
{
    return m_currentCharges > 0;
}

void LyrielDash::onDashStarted()
{
    --m_currentCharges;
}

void LyrielDash::onDashUpdate(float dt)
{
    if (m_currentCharges >= m_maxCharges)
    {
        return;
    }

    m_chargeRecoveryTimer += dt;

    while (m_chargeRecoveryTimer >= m_chargeRechargeTime && m_currentCharges < m_maxCharges)
    {
        ++m_currentCharges;
        m_chargeRecoveryTimer -= m_chargeRechargeTime;
    }

    if (m_currentCharges >= m_maxCharges)
    {
        m_currentCharges = m_maxCharges;
        m_chargeRecoveryTimer = 0.0f;
    }
}

IMPLEMENT_SCRIPT(LyrielDash)