#pragma once
#include "EditorWindow.h"
#include <string>

class ModuleScene;
class ModuleMusic;

class GameObject;

class SceneConfig : public EditorWindow
{

private:

    ModuleScene* m_moduleScene;
    ModuleMusic* m_moduleMusic;

    std::string m_saveSceneName;
    std::string m_loadSceneName;
    GameObject* m_camera = nullptr;
    bool m_skyboxDirty = false;
    bool m_lightingDirty = false;

public:

    SceneConfig();

    ~SceneConfig() = default;

    const char* getWindowName() const override
    {
        return "Scene Configuration";
    }

    void drawInternal() override;

private:

    void drawSaveSceneSettings();

    void drawLoadSceneSettings();

    void drawNavmeshSettings();

    void drawSkyBoxSettings();

    void drawLightSettings();

    void drawMusicBanksSettings();
};
