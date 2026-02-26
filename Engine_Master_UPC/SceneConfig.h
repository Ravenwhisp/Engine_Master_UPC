#pragma once
#include "EditorWindow.h"
#include <string>

class SceneModule;
class GameObject;

class SceneConfig : public EditorWindow
{
private:
	SceneModule* m_sceneModule;

public:
    SceneConfig();
    ~SceneConfig() = default;

    const char* getWindowName() const override { return "Scene Configuration"; }
    void render() override;

private:
    void drawSaveSceneSettings();
    void drawLoadSceneSettings();
    void drawSkyboxSettings();
    void drawLightSettings();

    std::string m_saveSceneName;
    std::string m_loadSceneName;
    GameObject* m_camera = nullptr;

    bool m_skyboxDirty = false;
    bool m_lightingDirty = false;
};
