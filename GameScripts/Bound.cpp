#include "pch.h"
#include "Bound.h"
#include "BoundConfig.h"
#include "Damageable.h"
#include "HeartbeatHaptic.h"

IMPLEMENT_SCRIPT_FIELDS(Bound,
    SERIALIZED_COMPONENT_REF(m_firstTarget, "Player 1 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_secondTarget, "Player 2 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_BoundUI, "Bound UI", ComponentType::TRANSFORM),
    SERIALIZED_DATACONTAINER_REF(m_config, "Config"),
)

Bound::Bound(GameObject* owner) : Script(owner)
{

}

void Bound::Start()
{
    if (m_config.dataContainer)
    {
        BoundConfig* cfg = m_config.get();
        m_minDistance = cfg->m_minDistance;
        m_distanceDamage = cfg->m_distanceDamage;
        m_distanceInstaKill = cfg->m_distanceInstaKill;
        m_radiusThreshold = cfg->m_radiusThreshold;
        baseDamage = cfg->m_baseDamage;
        maxDamage = cfg->m_maxDamage;
    }

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
}

void Bound::Update()
{
    if (!m_firstTarget.getReferencedComponent() || !m_secondTarget.getReferencedComponent() ||
        !m_firstDamageable || !m_secondDamageable)
        return;

    const Vector3 p1 = TransformAPI::getPosition(m_firstTarget.getReferencedComponent());
    const Vector3 p2 = TransformAPI::getPosition(m_secondTarget.getReferencedComponent());

    // Midpoint
    m_center = (p1 + p2) * 0.5f;
    if (m_BoundUI.getReferencedComponent())
    {
        TransformAPI::setPosition(m_BoundUI.getReferencedComponent(), m_center);
    }

    const float distance = Vector3::Distance(p1, p2);

    const float maxRadius = m_distanceInstaKill * 0.5f;
    m_currentRadius = min(distance * 0.5f + m_radiusThreshold, maxRadius);

    if (m_previousDistance < m_distanceInstaKill && distance >= m_distanceInstaKill)
    {
        m_firstDamageable->takeDamage(m_firstDamageable->getCurrentHp());
        m_secondDamageable->takeDamage(m_secondDamageable->getCurrentHp());
        return;
    }


    if (distance > m_distanceDamage && distance < m_distanceInstaKill)
    {
        const float range = m_distanceInstaKill - m_minDistance;

        // Manual clamp
        float t = (distance - m_minDistance) / range;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        t = 0.45f + (t * t * 0.55f);

        // Linear damage scale
        const float damagePerSecond = baseDamage + (maxDamage - baseDamage) * t;

        const float damage = damagePerSecond * Time::getDeltaTime();

        m_firstDamageable->takeDamage(damage);
        m_secondDamageable->takeDamage(damage);

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
        if (m_haptic) m_haptic->stop();
    }

    m_previousDistance = distance;
}

void Bound::drawGizmo()
{
    if (!m_firstTarget.getReferencedComponent() || !m_secondTarget.getReferencedComponent())
        return;

    const Vector3 p1 = TransformAPI::getPosition(m_firstTarget.getReferencedComponent());
    const Vector3 p2 = TransformAPI::getPosition(m_secondTarget.getReferencedComponent());

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