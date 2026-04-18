#pragma once

#include "ScriptAPI.h"

class TestSceneChangeButton : public Script
{
    DECLARE_SCRIPT(TestSceneChangeButton)

public:
    explicit TestSceneChangeButton(GameObject* owner);

    ScriptMethodList getExposedMethods() const override;

    void ChangeScene(const std::string& sceneName);
};