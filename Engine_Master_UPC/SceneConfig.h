#pragma once
#include "EditorWindow.h"

class SceneModule;
class GameObject;

class SceneConfig : public EditorWindow
{
private:
	SceneModule* m_sceneModule;

private:
	std::string m_sceneName;
	std::string m_skybox_path;
	GameObject* m_camera = nullptr;

public:
    SceneConfig();
    ~SceneConfig() = default;

    const char* getWindowName() const override { return "Scene Configuration"; }
    void render() override;
};
