#include "pch.h"
#include "LyrielDash.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"
#include "LyrielParticles.h"
#include "PlayerMovement.h"

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

    m_currentCharges = m_lyrielCharacter->getConfig()->m_dashMaxCharges;

    m_lyrielUI = GameObjectAPI::findScript<LyrielUI>(getOwner());

    m_sound = GameObjectAPI::findScript<LyrielSound>(getOwner());

    m_particles = GameObjectAPI::findScript<LyrielParticles>(getOwner());

    if (!m_particles)
    {
        Debug::error("[LyrielDash] LyrielParticles not found.");
        return;
    }
}

void LyrielDash::recoverCharge()
{
    if (m_currentCharges < m_lyrielCharacter->getConfig()->m_dashMaxCharges)
    {
        ++m_currentCharges;

        if (m_currentCharges == m_lyrielCharacter->getConfig()->m_dashMaxCharges)
        {
            m_chargeRecoveryTimer = 0.0f;
        }
    }
}

float LyrielDash::getCooldown() const
{
    return m_lyrielCharacter->getConfig()->m_dashCooldown;
}

float LyrielDash::getDashDuration() const
{
    return m_lyrielCharacter->getConfig()->m_dashDuration;
}

float LyrielDash::getDashDistance() const
{
    return m_lyrielCharacter->getConfig()->m_dashDistance;
}

bool LyrielDash::canDash() const
{
    return m_currentCharges > 0;
}

void LyrielDash::onDashStarted()
{
    --m_currentCharges;

    if (validateDashTarget())
    {
        m_playerMovement->m_playerType = static_cast<int>(NavAgentProfile::PlayerDash);
    }

    if (m_sound != nullptr)
    {
        m_sound->playDashWhoosh();
    }

    if (m_particles != nullptr)
    {
        m_particles->SetDashActive();
    }
}

void LyrielDash::onDashUpdate(float dt)
{
    if (m_currentCharges < m_lyrielCharacter->getConfig()->m_dashMaxCharges)
    {
        m_chargeRecoveryTimer += dt;

        while (m_chargeRecoveryTimer >= m_lyrielCharacter->getConfig()->m_dashRechargeTime && m_currentCharges < m_lyrielCharacter->getConfig()->m_dashMaxCharges)
        {
            ++m_currentCharges;
            m_chargeRecoveryTimer -= m_lyrielCharacter->getConfig()->m_dashRechargeTime;
        }

        if (m_currentCharges >= m_lyrielCharacter->getConfig()->m_dashMaxCharges)
        {
            m_currentCharges = m_lyrielCharacter->getConfig()->m_dashMaxCharges;
            m_chargeRecoveryTimer = 0.0f;
        }
    }

    if (m_lyrielUI)
    {
        m_lyrielUI->updateDashChargesUI(m_currentCharges, m_lyrielCharacter->getConfig()->m_dashMaxCharges, dt);
    }
}

void LyrielDash::onDashEnded()
{
    m_playerMovement->m_playerType = static_cast<int>(NavAgentProfile::PlayerNormal);

    if (m_particles != nullptr)
    {
        m_particles->SetDashInactive();
    }
}

bool LyrielDash::validateDashTarget()
{
    const Vector3 currentPosition = TransformAPI::getGlobalPosition(getOwner()->GetTransform());
    const Vector3 idealEnd = currentPosition + m_dashDirection * getDashDistance();
    m_hasDashTarget = false;
    m_debugDashStart = currentPosition;
    m_debugDashCandidateEnd = idealEnd;
    m_debugDashSampleEnd = idealEnd;
    m_debugLastDashValid = false;

    const Vector3 dashSearchExtents(0.2f, 2.0f, 0.2f);
    Vector3 candidateEnd;
    if (NavigationAPI::moveAlongSurface(currentPosition, idealEnd, candidateEnd, dashSearchExtents, NavAgentProfile::PlayerDash))
    {
        const float landingTolerance = m_lyrielCharacter->getConfig()->m_dashLandingTolerance;
        const Vector3 landingSearchExtents(landingTolerance, 2.0f, landingTolerance);
        Vector3 checkEnd;
        if (NavigationAPI::samplePosition(candidateEnd, checkEnd, landingSearchExtents, NavAgentProfile::PlayerNormal))
        {
            // A nearby landing must still be reachable along the dash navmesh.
            Vector3 reachableEnd;
            if (!NavigationAPI::moveAlongSurface(currentPosition, checkEnd, reachableEnd, dashSearchExtents, NavAgentProfile::PlayerDash)
                || (reachableEnd - checkEnd).LengthSquared() > 0.05f * 0.05f)
            {
                return false;
            }

            m_dashTargetPosition = checkEnd;
            m_hasDashTarget = true;

            m_debugDashSampleEnd = checkEnd;      // Debugging
            m_debugLastDashValid = true;         // Debugging
            return true;
        }
    }

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
