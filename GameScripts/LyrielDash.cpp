#include "pch.h"
#include "LyrielDash.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"

LyrielDash::LyrielDash(GameObject* owner)
    : AbilityDash(owner)
{
}

void LyrielDash::Start()
{
    AbilityDash::Start();

    m_lyrielCharacter = dynamic_cast<LyrielCharacter*>(m_character);

    if (!m_lyrielCharacter)
    {
        Debug::error("[LyrielDash] LyrielCharacter not found.");
        return;
    }

    m_config = GameObjectAPI::findScript<LyrielConfig>(getOwner());

    m_currentCharges = m_config->m_dashMaxCharges;

    m_lyrielUI = GameObjectAPI::findScript<LyrielUI>(getOwner());

    if (!m_lyrielUI)
    {
        Debug::warn("[LyrielDash] LyrielUI not found.");
    }
    else
    {
        m_lyrielUI->setupDashCharges(m_config->m_dashMaxCharges);
    }

    m_sound = GameObjectAPI::findScript<LyrielSound>(getOwner());
}

void LyrielDash::recoverCharge()
{
    if (m_currentCharges < m_config->m_dashMaxCharges)
    {
        ++m_currentCharges;

        if (m_currentCharges == m_config->m_dashMaxCharges)
        {
            m_chargeRecoveryTimer = 0.0f;
        }
    }
}

float LyrielDash::getCooldown() const
{
    return m_config->m_dashCooldown;
}

float LyrielDash::getDashDuration() const
{
    return m_config->m_dashDuration;
}

float LyrielDash::getDashDistance() const
{
    return m_config->m_dashDistance;
}

bool LyrielDash::canDash() const
{
    return m_currentCharges > 0;
}

void LyrielDash::onDashStarted()
{
    --m_currentCharges;

    if (m_sound != nullptr)
    {
        m_sound->playDashWhoosh();
    }
}

void LyrielDash::onDashUpdate(float dt)
{
    if (m_currentCharges < m_config->m_dashMaxCharges)
    {
        m_chargeRecoveryTimer += dt;

        while (m_chargeRecoveryTimer >= m_config->m_dashRechargeTime && m_currentCharges < m_config->m_dashMaxCharges)
        {
            ++m_currentCharges;
            m_chargeRecoveryTimer -= m_config->m_dashRechargeTime;
        }

        if (m_currentCharges >= m_config->m_dashMaxCharges)
        {
            m_currentCharges = m_config->m_dashMaxCharges;
            m_chargeRecoveryTimer = 0.0f;
        }
    }

    if (m_lyrielUI)
    {
        m_lyrielUI->updateDashChargesUI(m_currentCharges, m_config->m_dashMaxCharges, dt);
    }
}

bool LyrielDash::validateDashTarget()
{
    Vector3 currentPosition = TransformAPI::getPosition(getOwner()->GetTransform());
    m_debugDashStart = currentPosition; // Debugging

    Vector3 candidateEnd = currentPosition + m_dashDirection * getDashDistance();
    m_debugDashCandidateEnd = candidateEnd; // Debugging

    Vector3 sampledPosition;
    Vector3 searchExtents = Vector3(1.0f, 2.0f, 1.0f);

    if (NavigationAPI::samplePosition(candidateEnd, sampledPosition, searchExtents, NavAgentProfile::PlayerNormal))
    {
        m_dashTargetPosition = sampledPosition;
        m_hasDashTarget = true;
        m_debugDashSampleEnd = sampledPosition; // Debugging
        m_debugLastDashValid = true; // Debugging

        return true;
    }

    m_debugLastDashValid = false; // Debugging
    return false;
}

void LyrielDash::drawGizmo()
{
    const Vector3 white = { 1.0f, 1.0f, 1.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };
    const Vector3 cyan = { 0.0f, 1.0f, 1.0f };
    const Vector3 red = { 1.0f, 0.0f, 0.0f };
    const Vector3 up = { 0.0f, 1.0f, 0.0f };

    DebugDrawAPI::drawArrow(m_debugDashStart, m_debugDashCandidateEnd, white, 0.25f);
    DebugDrawAPI::drawCircle(m_debugDashCandidateEnd, up, yellow, 0.45f);

    if (m_debugLastDashValid)
    {
        DebugDrawAPI::drawCircle(m_debugDashSampleEnd, up, cyan, 0.35f);
    }
    else
    {
        DebugDrawAPI::drawCircle(m_debugDashCandidateEnd, up, red, 0.55f);
    }
}

IMPLEMENT_SCRIPT(LyrielDash)