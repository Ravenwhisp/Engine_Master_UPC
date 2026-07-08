#include "pch.h"
#include "Bound.h"
#include "Damageable.h"
#include "HeartbeatHaptic.h"
#include "CooperativeSound.h"

IMPLEMENT_SCRIPT_FIELDS(Bound,
    SERIALIZED_COMPONENT_REF(m_firstTarget, "Player 1 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_secondTarget, "Player 2 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_BoundUI, "Bound UI", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_minDistance, "Min Distance", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(m_distanceDamage, "Damage Distance", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(m_distanceInstaKill, "InstaKill Distance", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(m_radiusThreshold, "Radius Threshold", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(baseDamage, "Base Damage", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(maxDamage, "Max Damage", 0.0f, 0.0f, 0.1f),
    SERIALIZED_FLOAT(m_separationHapticHpGate, "Separation Haptic HP Gate", 0.5f, 0.25f, 0.01f)
)

Bound::Bound(GameObject* owner) : Script(owner)
{

}

void Bound::Start()
{
    GameObject* player1 = ComponentAPI::getOwner(m_firstTarget.getReferencedComponent());
    GameObject* player2 = ComponentAPI::getOwner(m_secondTarget.getReferencedComponent());

    if (player1 != nullptr)
    {
        m_firstDamageable = GameObjectAPI::findScript<Damageable>(player1);
    }

    if (player2 != nullptr)
    {
        m_secondDamageable = GameObjectAPI::findScript<Damageable>(player2);
    }

    m_haptic = GameObjectAPI::findScript<HeartbeatHaptic>(m_owner);

    if (m_haptic != nullptr)
    {
        m_haptic->m_variant = HapticEffectDefinition::HeartbeatVariant::Separation;
    }

    const auto coopGOs = SceneAPI::findAllGameObjectsWithScript<CooperativeSound>();
    if (!coopGOs.empty())
    {
        m_coopSound = GameObjectAPI::findScript<CooperativeSound>(coopGOs.front());
    }
}

void Bound::Update()
{
    if (!m_firstTarget.getReferencedComponent() || !m_secondTarget.getReferencedComponent() ||
        !m_firstDamageable || !m_secondDamageable)
        return;

    const Vector3 p1 = TransformAPI::getGlobalPosition(m_firstTarget.getReferencedComponent());
    const Vector3 p2 = TransformAPI::getGlobalPosition(m_secondTarget.getReferencedComponent());

    // Midpoint
    m_center = (p1 + p2) * 0.5f;
    if (m_BoundUI.getReferencedComponent())
    {
        TransformAPI::setGlobalPosition(m_BoundUI.getReferencedComponent(), m_center);
    }

    const float distance = Vector3::Distance(p1, p2);

    const float maxRadius = m_distanceInstaKill * 0.5f;
    m_currentRadius = min(distance * 0.5f + m_radiusThreshold, maxRadius);

    if (m_previousDistance < m_distanceInstaKill && distance >= m_distanceInstaKill)
    {
        if (m_coopSound) m_coopSound->stopBoundDamageLoop();
        m_firstDamageable->takeDamage(m_firstDamageable->getCurrentHp());
        m_secondDamageable->takeDamage(m_secondDamageable->getCurrentHp());
        return;
    }


    if (distance > m_distanceDamage && distance < m_distanceInstaKill)
    {
        // Separation damage band: the cooperative loop replaces the per-hit hurt grunt.
        if (m_coopSound) m_coopSound->startBoundDamageLoop();

        const float range = m_distanceInstaKill - m_minDistance;

        // Manual clamp
        float t = (distance - m_minDistance) / range;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        t = 0.45f + (t * t * 0.55f);

        // Linear damage scale
        const float damagePerSecond = baseDamage + (maxDamage - baseDamage) * t;

        const float damage = damagePerSecond * Time::getDeltaTime();

        // Continuous flag: suppresses the per-hit hurt grunt (the Bound-Damage loop
        // above conveys the separation); the escalating heartbeat handles the tension.
        m_firstDamageable->takeDamage(HitContext{ damage, /*continuous=*/true });
        m_secondDamageable->takeDamage(HitContext{ damage, /*continuous=*/true });

        const bool p1LowHp = m_firstDamageable->getHpPercent() < m_separationHapticHpGate;
        const bool p2LowHp = m_secondDamageable->getHpPercent() < m_separationHapticHpGate;

        if (m_haptic)
        {
            if (p1LowHp && p2LowHp)
                m_haptic->tick(t);
            else
                m_haptic->stop();
        }
    }
    else
    {
        if (m_coopSound) m_coopSound->stopBoundDamageLoop();
        if (m_haptic) m_haptic->stop();
    }

    m_previousDistance = distance;
}

void Bound::drawGizmo()
{
    if (!m_firstTarget.getReferencedComponent() || !m_secondTarget.getReferencedComponent())
        return;

    const Vector3 p1 = TransformAPI::getGlobalPosition(m_firstTarget.getReferencedComponent());
    const Vector3 p2 = TransformAPI::getGlobalPosition(m_secondTarget.getReferencedComponent());

    const Vector3 center = (p1 + p2) * 0.5f;
    const float distance = Vector3::Distance(p1, p2);
    const Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    // Recompute radius locally
    const float maxRadius = m_distanceInstaKill * 0.5f;
    const float currentRadius = min(distance * 0.5f + m_radiusThreshold, maxRadius);

    DebugDrawAPI::drawLine(p1, p2, Vector3(1.0f, 1.0f, 1.0f));
    DebugDrawAPI::drawPoint(center, Vector3(1.0f, 1.0f, 0.0f), 4.0f);

    Vector3 circleColor;
    if (distance <= m_minDistance)
        circleColor = Vector3(0.0f, 1.0f, 0.0f);
    else if (distance < m_distanceDamage)
        circleColor = Vector3(1.0f, 1.0f, 0.0f);
    else if (distance < m_distanceInstaKill)
        circleColor = Vector3(1.0f, 0.3f, 0.0f);
    else
        circleColor = Vector3(1.0f, 1.0f, 1.0f);

    DebugDrawAPI::drawCircle(center, up, circleColor, currentRadius, 32.0f);

    const float damageRadius = min(m_distanceDamage * 0.5f, m_distanceInstaKill * 0.5f);
    const float instaKillRadius = m_distanceInstaKill * 0.5f;

    DebugDrawAPI::drawCircle(center, up, Vector3(1.0f, 1.0f, 0.0f), damageRadius, 32.0f);
    DebugDrawAPI::drawCircle(center, up, Vector3(1.0f, 0.0f, 0.0f), instaKillRadius, 32.0f);
}


IMPLEMENT_SCRIPT(Bound)