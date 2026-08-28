#pragma once

#include "ScriptAPI.h"

class MouseAimTest : public Script
{
    DECLARE_SCRIPT(MouseAimTest)

public:
    explicit MouseAimTest(GameObject* owner);

    void Update() override;

    FieldList getExposedFields() const override;

public:
    int m_playerIndex = 0;
    float m_arrowLength = 3.0f;
    float m_arrowSize = 0.2f;
};
