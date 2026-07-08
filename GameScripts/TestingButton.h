#pragma once

#include "ScriptAPI.h"

class TestingButton : public Script
{
    DECLARE_SCRIPT(TestingButton)

public:
    explicit TestingButton(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptMethodList getExposedMethods() const override;

    void onFieldEdited(const ScriptFieldInfo& field) override;
    void onAfterDeserialize() override;

public:
    void ButtonHover();
    void ButtonPress();
    void ButtonRelease();
};
