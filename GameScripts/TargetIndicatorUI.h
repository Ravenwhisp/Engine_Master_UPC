#pragma once

#include "ScriptAPI.h"

class Transform;
class PlayerTargetController;

class TargetIndicatorUI : public Script
{
    DECLARE_SCRIPT(TargetIndicatorUI)

public:
    explicit TargetIndicatorUI(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

protected:
    virtual void onStart() {}
    virtual void updateDirectionIndicator(GameObject* currentTarget) {}
    virtual void hideDirectionIndicator() {}

    PlayerTargetController* getPlayerTargetController() const;

    // Target Indicator animation
    void updateTargetIndicator(GameObject* currentTarget);

    void hideTargetIndicator();
    void showTargetIndicator();

    void startSwitchAnimation();
    void updateSwitchAnimation(Transform* targetIndicatorTransform);

    // Direction indicator helpers
    bool tryGetFlatDirectionToTarget(GameObject* currentTarget, Vector3& outPlayerPosition, Vector3& outDirection, float* outDistance = nullptr) const;

    void setVisualActive(Transform* visualTransform, bool active) const;
    float getDirectionAngleDegrees(const Vector3& direction, float rotationOffsetDegrees) const;

protected:
    ComponentRef<Transform> m_playerTransform;
    ComponentRef<Transform> m_targetIndicatorTransform;

    Vector3 m_positionOffset = Vector3(0.0f, 0.05f, 0.0f);
    float m_followSharpness = 20.0f;

    float m_switchPopScale = 1.25f;
    float m_switchPopDuration = 0.18f;

    PlayerTargetController* m_playerTargetController = nullptr;
    GameObject* m_previousTarget = nullptr;

    Vector3 m_targetIndicatorBaseScale = Vector3(1.0f, 1.0f, 1.0f);
    float m_switchAnimationTimer = 0.0f;
};