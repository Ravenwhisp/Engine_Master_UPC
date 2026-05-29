#include "pch.h"
#include "LyrielDash.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielDash, AbilityDash,
    SERIALIZED_FLOAT(m_chargeRechargeTime, "Charge Recharge Time", 0.1f, 10.0f, 0.1f),
    SERIALIZED_INT(m_maxCharges, "Max charges"),
	SERIALIZED_COMPONENT_REF(m_charge1UI, "Charge 1 UI", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_charge2UI, "Charge 2 UI", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_charge3UI, "Charge 3 UI", ComponentType::TRANSFORM2D),
    SERIALIZED_FLOAT(chargedScale, "Charged Scale", 0.1f, 5.0f, 0.1f),
    SERIALIZED_FLOAT(emptyScale, "Empty Scale", 0.1f, 5.0f, 0.1f),
	SERIALIZED_FLOAT(uiScaleSpeed, "UI Scale Speed", 0.1f, 20.0f, 0.1f)
)

LyrielDash::LyrielDash(GameObject* owner)
    : AbilityDash(owner)
{
}

void LyrielDash::Start()
{
    AbilityDash::Start();

    if (m_character == nullptr)
    {
        Debug::log("[LyrielDash] CharacterBase not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    m_currentCharges = m_maxCharges;

	m_charge1Transform2D = m_charge1UI.getReferencedComponent();
	m_charge2Transform2D = m_charge2UI.getReferencedComponent();
	m_charge3Transform2D = m_charge3UI.getReferencedComponent();

    m_sound = GameObjectAPI::findScript<LyrielSound>(getOwner());

    AbilityDash::Start();
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

    if (m_sound != nullptr)
    {
        m_sound->playDashWhoosh();
    }
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

bool LyrielDash::validateDashTarget()
{
    Vector3 currentPosition = TransformAPI::getPosition(getOwner()->GetTransform());
    m_debugDashStart = currentPosition; // Debugging

    Vector3 candidateEnd = currentPosition + m_dashDirection * m_dashDistance;
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

void LyrielDash::updateUI()
{
    float target1 = (m_currentCharges >= 1) ? chargedScale : emptyScale;
    float target2 = (m_currentCharges >= 2) ? chargedScale : emptyScale;
    float target3 = (m_currentCharges >= 3) ? chargedScale : emptyScale;

    float dt = Time::getDeltaTime();

    updateChargeVisual(m_charge1Transform2D, m_charge1Scale, target1, dt);
    updateChargeVisual(m_charge2Transform2D, m_charge2Scale, target2, dt);
    updateChargeVisual(m_charge3Transform2D, m_charge3Scale, target3, dt);

    if (m_cooldownTimer <= 0.0f && m_currentCharges > 0)
    {
        if (m_cdGO)
        {
            GameObjectAPI::setActive(m_cdGO, false);
        }
        return;
    }
    if (m_cdBarSlider)
    {
        SliderAPI::setFillAmount(m_cdBarSlider, (m_cooldownTimer / m_cooldown));
    }
}

float LyrielDash::moveTowards(float current, float target, float maxDelta)
{
    float delta = target - current;

    if (fabs(delta) <= maxDelta)
    {
        return target;
    }

    return current + (delta > 0.0f ? maxDelta : -maxDelta);
}

void LyrielDash::updateChargeVisual(Transform2D* transform, float& currentScale, float targetScale, float dt)
{
    if (!transform)
    {
        return;
    }

    currentScale = moveTowards(currentScale, targetScale, uiScaleSpeed * dt);
	Transform2DAPI::setScale(transform, Vector2(currentScale, currentScale));
}

IMPLEMENT_SCRIPT(LyrielDash)