#include "pch.h"
#include "Bound.h"
#include "Damageable.h"
#include "HeartbeatHaptic.h"
#include "CooperativeSound.h"
#include "BoundConfig.h"
#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS(Bound,
    SERIALIZED_COMPONENT_REF(m_firstTarget, "Player 1 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_secondTarget, "Player 2 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_boundUI, "Bound UI", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_boundaryWarningUI, "Boundary Warning UI", ComponentType::TRANSFORM2D),
    SERIALIZED_ASSET_REF(m_config, "Bound Config", AssetType::DATA_CONTAINER)
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

    if (m_config.get())
    {
        const BoundConfig* cfg = m_config.get();
        m_minDistance = cfg->m_minDistance;
        m_showBoundDistance = cfg->m_showBoundDistance;
        baseDamage = cfg->m_baseDamage;
        m_radiusThreshold = cfg->m_radiusThreshold;
        m_separationHapticHpGate = cfg->m_separationHapticHpGate;
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
    if (m_boundUI.getReferencedComponent())
    {
        TransformAPI::setGlobalPosition(m_boundUI.getReferencedComponent(), m_center);
    }

    const float distance = Vector3::Distance(p1, p2);

    m_currentRadius = distance * 0.5f + m_radiusThreshold;

    // --- Boundary warning alpha ---
    if (m_boundaryWarningUI.getReferencedComponent())
    {
        float alpha = 0.0f;
        const float alphaRange = m_minDistance - m_showBoundDistance;
        const float alphaMid = (m_showBoundDistance + alphaRange * 0.5f) ;
        if (distance > m_showBoundDistance && alphaRange > 0.0f)
        {
            if (distance >= alphaMid)
                alpha = 1.0f;
            else
                alpha = (distance - m_showBoundDistance) / (alphaRange * 0.5f);
        }
        Transform2DAPI::setAlpha(m_boundaryWarningUI.getReferencedComponent(), alpha - 0.4);
    }

    // --- Exponential damage beyond safe zone ---
    if (distance > m_minDistance)
    {
        if (m_coopSound) m_coopSound->startBoundDamageLoop();

        const float excess = (distance - m_minDistance) / m_minDistance;
        const float damagePerSecond = baseDamage * pow(2.0f, excess);
        const float damage = damagePerSecond * Time::getDeltaTime();

        m_firstDamageable->takeDamage(HitContext{ damage, true });
        m_secondDamageable->takeDamage(HitContext{ damage, true });

        const bool p1LowHp = m_firstDamageable->getHpPercent() < m_separationHapticHpGate;
        const bool p2LowHp = m_secondDamageable->getHpPercent() < m_separationHapticHpGate;

        if (m_haptic)
        {
            if (p1LowHp && p2LowHp)
                m_haptic->tick(min(excess, 1.0f));
            else
                m_haptic->stop();
        }
    }
    else
    {
        if (m_coopSound) m_coopSound->stopBoundDamageLoop();
        if (m_haptic) m_haptic->stop();
    }
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

    const float currentRadius = distance * 0.5f + m_radiusThreshold;

    DebugDrawAPI::drawLine(p1, p2, Vector3(1.0f, 1.0f, 1.0f));
    DebugDrawAPI::drawPoint(center, Vector3(1.0f, 1.0f, 0.0f), 4.0f);

    const Vector3 circleColor = distance <= m_minDistance
        ? Vector3(0.0f, 1.0f, 0.0f)
        : Vector3(1.0f, 0.3f, 0.0f);

    DebugDrawAPI::drawCircle(center, up, circleColor, currentRadius, 32.0f);
}


IMPLEMENT_SCRIPT(Bound)