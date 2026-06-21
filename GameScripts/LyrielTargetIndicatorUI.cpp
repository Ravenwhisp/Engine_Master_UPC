#include "pch.h"
#include "LyrielTargetIndicatorUI.h"

#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielTargetIndicatorUI, TargetIndicatorUI,
    FIELD_GROUP_LABEL("Direction Arrow"),
    SERIALIZED_COMPONENT_REF(m_directionArrowTransform, "Direction Arrow Transform", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_forwardOffset, "Forward Offset", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", -1.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_rotationOffsetDegrees, "Rotation Offset Degrees", -180.0f, 180.0f, 1.0f),
    SERIALIZED_FLOAT(m_minDistanceToShowArrow, "Min Distance To Show Arrow", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_fadeDuration, "Fade Duration", 0.01f, 1.0f, 0.01f)
)

LyrielTargetIndicatorUI::LyrielTargetIndicatorUI(GameObject* owner)
    : TargetIndicatorUI(owner)
{
}

void LyrielTargetIndicatorUI::updateDirectionIndicator(GameObject* currentTarget)
{
    Transform* arrowTransform = m_directionArrowTransform.getReferencedComponent();
    if (arrowTransform == nullptr)
    {
        hideDirectionIndicator();
        return;
    }

    Vector3 playerPosition;
    Vector3 direction;
    float distanceToTarget = 0.0f;

    if (!tryGetFlatDirectionToTarget(currentTarget, playerPosition, direction, &distanceToTarget))
    {
        hideDirectionIndicator();
        return;
    }

    const bool shouldShowArrow = distanceToTarget > m_minDistanceToShowArrow;

    if (shouldShowArrow)
    {
        setVisualActive(arrowTransform, true);
        updateDirectionArrowTransform(arrowTransform, playerPosition, direction);
    }

    updateDirectionArrowVisibility(shouldShowArrow);
}

void LyrielTargetIndicatorUI::hideDirectionIndicator()
{
    Transform2D* arrowTransform2D = getDirectionArrowTransform2D();
    if (arrowTransform2D != nullptr)
    {
        Transform2DAPI::setAlpha(arrowTransform2D, 0.0f);
    }

    Transform* arrowTransform = m_directionArrowTransform.getReferencedComponent();
    setVisualActive(arrowTransform, false);
}

void LyrielTargetIndicatorUI::updateDirectionArrowTransform(Transform* arrowTransform, const Vector3& playerPosition, const Vector3& direction) const
{
    if (arrowTransform == nullptr)
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

    Vector3 arrowPosition = playerPosition + flatDirection * m_forwardOffset;
    arrowPosition.y = playerPosition.y + m_heightOffset;

    TransformAPI::setGlobalPosition(arrowTransform, arrowPosition);

    const float angleDegrees = getDirectionAngleDegrees(flatDirection, m_rotationOffsetDegrees);
    TransformAPI::setRotationEuler(arrowTransform, Vector3(0.0f, 0.0f, angleDegrees));
}

void LyrielTargetIndicatorUI::updateDirectionArrowVisibility(bool shouldBeVisible)
{
    Transform2D* arrowTransform2D = getDirectionArrowTransform2D();
    Transform* arrowTransform = m_directionArrowTransform.getReferencedComponent();

    if (arrowTransform2D == nullptr)
    {
        setVisualActive(arrowTransform, shouldBeVisible);
        return;
    }

    if (shouldBeVisible)
    {
        setVisualActive(arrowTransform, true);
    }

    const float currentAlpha = Transform2DAPI::getAlpha(arrowTransform2D);
    const float targetAlpha = shouldBeVisible ? 1.0f : 0.0f;

    float fadeStep = 1.0f;
    if (m_fadeDuration > 0.0001f)
    {
        fadeStep = Time::getDeltaTime() / m_fadeDuration;
    }

    const float newAlpha = MathAPI::moveTowards(currentAlpha, targetAlpha, fadeStep);
    Transform2DAPI::setAlpha(arrowTransform2D, newAlpha);

    if (!shouldBeVisible && newAlpha <= 0.001f)
    {
        setVisualActive(arrowTransform, false);
    }
}

Transform2D* LyrielTargetIndicatorUI::getDirectionArrowTransform2D() const
{
    Transform* arrowTransform = m_directionArrowTransform.getReferencedComponent();
    if (arrowTransform == nullptr)
    {
        return nullptr;
    }

    GameObject* arrowObject = ComponentAPI::getOwner(arrowTransform);

    Component* transform2DComponent = GameObjectAPI::getComponent(arrowObject, ComponentType::TRANSFORM2D);
    return static_cast<Transform2D*>(transform2DComponent);
}

IMPLEMENT_SCRIPT(LyrielTargetIndicatorUI)