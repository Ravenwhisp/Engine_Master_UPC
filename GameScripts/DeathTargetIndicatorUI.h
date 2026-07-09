#pragma once

#include "TargetIndicatorUI.h"

class DeathConfig;

class DeathTargetIndicatorUI : public TargetIndicatorUI
{
    DECLARE_SCRIPT(DeathTargetIndicatorUI)

public:
    explicit DeathTargetIndicatorUI(GameObject* owner);

    FieldList getExposedFields() const override;

protected:
    void onStart() override;
    void updateDirectionIndicator(GameObject* currentTarget) override;
    void hideDirectionIndicator() override;

private:
    void updateRangeIndicatorTransform(Transform* rangeTransform, const Vector3& playerPosition, const Vector3& direction) const;

public:
    ComponentRef<Transform> m_rangeIndicatorTransform;

    float m_heightOffset = 0.05f;
    float m_rotationOffsetDegrees = -90.0f;

    Vector3 m_rangeIndicatorFullScale = Vector3(1.0f, 1.0f, 1.0f);

private:
    DeathConfig* m_deathConfig = nullptr;
};