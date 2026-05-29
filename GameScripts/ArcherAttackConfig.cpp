#include "pch.h"
#include "ArcherAttackConfig.h"

#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS(ArcherAttackConfig,
    // Basic Attack
    SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
    SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f),
    // Arrow Barrage
    SERIALIZED_FLOAT(m_arrowBarrageRange, "Arrow Barrage Range", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_arrowBarrageDamage, "Arrow Barrage Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_arrowBarrageRadius, "Arrow Barrage Radius", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_arrowBarrageThrowTime, "Arrow Barrage Throw Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_arrowBarrageLandDelay, "Arrow Barrage Land Delay", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_arrowBarrageTotalDuration, "Arrow Barrage Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_arrowBarrageCooldown, "Arrow Barrage Cooldown", 0.0f, 30.0f, 0.1f),
    // Somersault
    SERIALIZED_FLOAT(m_somersaultTriggerRange, "Somersault Trigger Range", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_somersaultDistance, "Somersault Distance", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_somersaultDuration, "Somersault Duration", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_somersaultCooldown, "Somersault Cooldown", 0.0f, 30.0f, 0.1f)
)

ArcherAttackConfig::ArcherAttackConfig(GameObject* owner)
    : Script(owner)
{
}

void ArcherAttackConfig::Start()
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t)
    {
        m_barrageUITransform = TransformAPI::findChildByName(t, "ArrowBarrageUI");
        if (m_barrageUITransform)
        {
            GameObjectAPI::setActive(m_barrageUITransform->getOwner(), true);

            m_barrageUITransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(m_barrageUITransform->getOwner(), ComponentType::TRANSFORM2D));
            if (m_barrageUITransform2D)
            {
                Transform2DAPI::setAlpha(m_barrageUITransform2D, 0.0f);
                const float radius = m_arrowBarrageRadius;
                Transform2DAPI::setScale(m_barrageUITransform2D, Vector2(radius, radius));
            }

            Transform* glowTransform = TransformAPI::findChildByName(m_barrageUITransform, "Glow");
            if (glowTransform)
            {
                m_barrageUIGlow = static_cast<Transform2D*>(GameObjectAPI::getComponent(glowTransform->getOwner(), ComponentType::TRANSFORM2D));
                Transform2DAPI::setAlpha(m_barrageUIGlow, 0.0f);
            }
        }
    }

    if (!m_barrageUITransform)
    {
        Debug::warn("ArcherArrowBarrageState on '%s' could not find ArrowBarrageUI child for attack UI.", GameObjectAPI::getName(getOwner()));
    }
    else
    {
        if (!m_barrageUITransform2D)
        {
            Debug::warn("ArcherArrowBarrageState on '%s' could not find Transform2D on ArrowBarrageUI for attack UI.", GameObjectAPI::getName(getOwner()));
        }
        if (!m_barrageUIGlow)
        {
            Debug::warn("ArcherArrowBarrageState on '%s' could not find ArrowBarrageUIGlow child for attack UI.", GameObjectAPI::getName(getOwner()));
        }
    }
}

IMPLEMENT_SCRIPT(ArcherAttackConfig)