#include "pch.h"
#include "LyrielDash.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"
#include "LyrielParticles.h"
#include "PlayerMovement.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielDash,
    SERIALIZED_ASSET_REF(m_config, "Lyriel Config", AssetType::DATA_CONTAINER)
)

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

    const LyrielConfig* cfg = m_config.get();

    m_currentCharges = cfg->m_dashMaxCharges;

    m_lyrielUI = GameObjectAPI::findScript<LyrielUI>(getOwner());

    if (!m_lyrielUI)
    {
        Debug::warn("[LyrielDash] LyrielUI not found.");
    }
    else
    {
        m_lyrielUI->setupDashCharges(cfg->m_dashMaxCharges);
    }

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
    const LyrielConfig* cfg = m_config.get();
    if (!cfg) return;

    if (m_currentCharges < cfg->m_dashMaxCharges)
    {
        ++m_currentCharges;

        if (m_currentCharges == cfg->m_dashMaxCharges)
        {
            m_chargeRecoveryTimer = 0.0f;
        }
    }
}

float LyrielDash::getCooldown() const
{
    const LyrielConfig* cfg = m_config.get();
    return cfg ? cfg->m_dashCooldown : 0.0f;
}

float LyrielDash::getDashDuration() const
{
    const LyrielConfig* cfg = m_config.get();
    return cfg ? cfg->m_dashDuration : 0.0f;
}

float LyrielDash::getDashDistance() const
{
    const LyrielConfig* cfg = m_config.get();
    return cfg ? cfg->m_dashDistance : 0.0f;
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
    const LyrielConfig* cfg = m_config.get();
    if (!cfg) return;

    if (m_currentCharges < cfg->m_dashMaxCharges)
    {
        m_chargeRecoveryTimer += dt;

        while (m_chargeRecoveryTimer >= cfg->m_dashRechargeTime && m_currentCharges < cfg->m_dashMaxCharges)
        {
            ++m_currentCharges;
            m_chargeRecoveryTimer -= cfg->m_dashRechargeTime;
        }

        if (m_currentCharges >= cfg->m_dashMaxCharges)
        {
            m_currentCharges = cfg->m_dashMaxCharges;
            m_chargeRecoveryTimer = 0.0f;
        }
    }

    if (m_lyrielUI)
    {
        m_lyrielUI->updateDashChargesUI(m_currentCharges, cfg->m_dashMaxCharges, dt);
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
    //Vector3 currentPosition = TransformAPI::getGlobalPosition(getOwner()->GetTransform());
    //m_debugDashStart = currentPosition; // Debugging

    //Vector3 candidateEnd = currentPosition + m_dashDirection * getDashDistance();
    //m_debugDashCandidateEnd = candidateEnd; // Debugging

    //Vector3 sampledPosition;
    //Vector3 searchExtents = Vector3(1.0f, 2.0f, 1.0f);

    //if (NavigationAPI::samplePosition(candidateEnd, sampledPosition, searchExtents, NavAgentProfile::PlayerNormal))
    //{
    //    m_dashTargetPosition = sampledPosition;
    //    m_hasDashTarget = true;
    //    m_debugDashSampleEnd = sampledPosition; // Debugging
    //    m_debugLastDashValid = true; // Debugging

    //    return true;
    //}

    //m_debugLastDashValid = false; // Debugging
    //return false;

    Vector3 currentPosition = TransformAPI::getPosition(getOwner()->GetTransform());

    Vector3 idealEnd = currentPosition + m_dashDirection * getDashDistance();

    Vector3 candidateEnd;
    Vector3 searchExtents = Vector3(0.2f, 2.0f, 0.2f);

    if (NavigationAPI::moveAlongSurface(currentPosition, idealEnd, candidateEnd, searchExtents, NavAgentProfile::PlayerDash))
    {
        Vector3 checkEnd;
        searchExtents = Vector3(0.2f, 2.0f, 0.2f);
        if (NavigationAPI::samplePosition(candidateEnd, checkEnd, searchExtents, NavAgentProfile::PlayerNormal))
        {
            m_dashTargetPosition = candidateEnd;

            m_debugDashSampleEnd = candidateEnd; // Debugging
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