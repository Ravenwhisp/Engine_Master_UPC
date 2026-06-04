#pragma once

#include "ScriptAPI.h"

class TransformGlobalTest : public Script
{
    DECLARE_SCRIPT(TransformGlobalTest)

public:
    explicit TransformGlobalTest(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    bool m_runSetGlobalPositionTest = false;
    bool m_runTranslateGlobalTest = false;
    bool m_runSetGlobalRotationTest = false;
    bool m_runLookAtTest = false;

    Vector3 m_targetGlobalPosition = Vector3(10.0f, 0.0f, 0.0f);
    Vector3 m_translateDelta = Vector3(0.0f, 0.0f, 2.0f);
    Vector3 m_targetGlobalEuler = Vector3(0.0f, 90.0f, 0.0f);
    Vector3 m_lookAtTargetWorldPosition = Vector3(10.0f, 0.0f, 10.0f);

private:
    bool m_loggedInitialState = false;
};