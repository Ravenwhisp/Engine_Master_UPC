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
    float m_heightOffset = 2.0f;
    float m_followSharpness = 20.0f;

private:
    PlayerTargetController* getPlayerTargetController() const;
    void hideIndicator();
    void showIndicator();
};