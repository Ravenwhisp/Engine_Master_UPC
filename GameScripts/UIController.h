#pragma once

#include "ScriptAPI.h"

class AssetId;

class UIController : public Script
{
    DECLARE_SCRIPT(UIController)

public:
    explicit UIController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptMethodList getExposedMethods() const override;

	void ChangeScene(const std::string& sceneName);
    void ChangeScene2(const AssetId& sceneName);
	void ExitApplication();
    void StartGame(const std::string& sceneName);
	void PauseGame(bool pause);
};