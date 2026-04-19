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

    ScriptFieldList getExposedFields() const override;

public:
    ScriptComponentRef<Transform> m_playerTransform;
    ScriptComponentRef<Transform> m_indicatorVisualTransform;

    Vector3 m_positionOffset = Vector3(0.0f, 0.05f, 0.0f);
    float m_followSharpness = 20.0f;

private:
    PlayerTargetController* getPlayerTargetController() const;
    void hideIndicator();
    void showIndicator();

private:
    PlayerTargetController* m_playerTargetController = nullptr;
    GameObject* m_previousTarget = nullptr;
};