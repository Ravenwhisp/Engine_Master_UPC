#pragma once

#include "ScriptAPI.h"

class GamepadInputTest : public Script
{
    DECLARE_SCRIPT(GamepadInputTest)

public:
    explicit GamepadInputTest(GameObject* owner);

    void Start() override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

public:
    int m_playerIndex = 0;
    float m_moveSpeed = 5.0f;

private:
    bool m_wasConnectedLastFrame = false;
    bool m_wasAPressedLastFrame = false;
};