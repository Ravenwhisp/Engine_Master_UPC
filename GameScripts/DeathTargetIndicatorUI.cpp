#include "pch.h"
#include "DeathTargetIndicatorUI.h"

#include "DeathConfig.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathTargetIndicatorUI, TargetIndicatorUI,
    SERIALIZED_ASSET_REF(m_deathConfig, "Death Config", AssetType::DATA_CONTAINER),
    FIELD_GROUP_LABEL("Range Indicator"),
    SERIALIZED_COMPONENT_REF(m_rangeIndicatorTransform, "Range Indicator Transform", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", -1.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_rotationOffsetDegrees, "Rotation Offset Degrees", -180.0f, 180.0f, 1.0f),
    SERIALIZED_VEC3(m_rangeIndicatorFullScale, "Range Indicator Full Scale")
)

DeathTargetIndicatorUI::DeathTargetIndicatorUI(GameObject* owner)
    : TargetIndicatorUI(owner)
{
}

void DeathTargetIndicatorUI::onStart()
{
    Transform* playerTransform = m_playerTransform.getReferencedComponent();
    if (playerTransform == nullptr)
    {
        return;
    }
}

void DeathTargetIndicatorUI::updateDirectionIndicator(GameObject* currentTarget)
{
    Transform* rangeTransform = m_rangeIndicatorTransform.getReferencedComponent();
    if (rangeTransform == nullptr)
    {
        hideDirectionIndicator();
        return;
    }

    const DeathConfig* deathCfg = m_deathConfig.get();
    if (deathCfg == nullptr)
    {
        hideDirectionIndicator();
        return;
    }

    Vector3 playerPosition;
    Vector3 direction;

    if (!tryGetFlatDirectionToTarget(currentTarget, playerPosition, direction))
    {
        hideDirectionIndicator();
        return;
    }

    setVisualActive(rangeTransform, true);
    updateRangeIndicatorTransform(rangeTransform, playerPosition, direction);
}

void DeathTargetIndicatorUI::hideDirectionIndicator()
{
    Transform* rangeTransform = m_rangeIndicatorTransform.getReferencedComponent();
    setVisualActive(rangeTransform, false);
}

void DeathTargetIndicatorUI::updateRangeIndicatorTransform(Transform* rangeTransform, const Vector3& playerPosition, const Vector3& direction) const
{
    const DeathConfig* deathCfg = m_deathConfig.get();
    if (rangeTransform == nullptr || deathCfg == nullptr)
    {
        return;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatDirection.Normalize();

    const float attackRange = deathCfg->m_basicAttackRange;

    Vector3 rangePosition = playerPosition + flatDirection * (attackRange * 0.5f);
    rangePosition.y = playerPosition.y + m_heightOffset;

    TransformAPI::setGlobalPosition(rangeTransform, rangePosition);

    const float angleDegrees = getDirectionAngleDegrees(flatDirection, m_rotationOffsetDegrees);
    TransformAPI::setRotationEuler(rangeTransform, Vector3(0.0f, 0.0f, angleDegrees));

    TransformAPI::setScale(rangeTransform, m_rangeIndicatorFullScale);
}

IMPLEMENT_SCRIPT(DeathTargetIndicatorUI)