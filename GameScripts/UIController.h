#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class AssetId;

class UIController : public Script
{
    DECLARE_SCRIPT(UIController)

public:
    explicit UIController(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;
    ScriptMethodList getExposedMethods() const override;

    void StartScene(const std::string& sceneName);
	void ChangeScene(const std::string& sceneName);
    void ChangeScene2(const AssetId& sceneName);
    void ChangeLevel();
	void ExitApplication();
    void StartGame(const std::string& sceneName);
	void PauseGame(bool pause);

    ComponentRef<Transform> m_menuLights;
    ComponentRef<Transform2D> m_blackBg;

private:
	GameObject* m_menuLightsGO = nullptr;
	Transform2D* m_blackBgTransform = nullptr;
	const float m_blackBgFadeDuration = 3.0f;
    float m_blackBgFadeTimer = 0.0f;
    bool m_isFading = false;
    std::string m_pendingSceneName;
	std::string m_preloadedLevelName;
};
