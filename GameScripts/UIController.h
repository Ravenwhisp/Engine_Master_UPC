#pragma once

#include "ScriptAPI.h"

class UIController : public Script
{
    DECLARE_SCRIPT(UIController)

public:
    explicit UIController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptMethodList getExposedMethods() const override;

	void ChangeScene(const std::string& sceneName);
	void ExitApplication();
	void PauseGame(bool pause);
};