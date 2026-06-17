#pragma once

#include "TargetIndicatorUI.h"

class Transform2D;

class LyrielTargetIndicatorUI : public TargetIndicatorUI
{
    DECLARE_SCRIPT(LyrielTargetIndicatorUI)

public:
    explicit LyrielTargetIndicatorUI(GameObject* owner);

    ScriptFieldList getExposedFields() const override;

protected:
    void updateDirectionIndicator(GameObject* currentTarget) override;
    void hideDirectionIndicator() override;

private:
    void updateDirectionArrowTransform(Transform* arrowTransform, const Vector3& playerPosition, const Vector3& direction) const;

    void updateDirectionArrowVisibility(bool shouldBeVisible);
    Transform2D* getDirectionArrowTransform2D() const;

public:
    ScriptComponentRef<Transform> m_directionArrowTransform;

    float m_forwardOffset = 1.0f;
    float m_heightOffset = 0.05f;
    float m_rotationOffsetDegrees = -90.0f;

    float m_minDistanceToShowArrow = 1.7f;
    float m_fadeDuration = 0.15f;

};