#pragma once
#include "Module.h"

#include <memory>
#include <string>
#include <rapidjson/document.h>

class Scene;
class Quadtree;
class SceneSerializer;

class GameObject;
class Component;
class CameraComponent;
class MeshRenderer;
class ScriptComponent;
class LightComponent;

struct ID3D12GraphicsCommandList;

class ModuleScene : public Module
{
private:
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Quadtree> m_quadtree;

    std::unique_ptr<SceneSerializer> m_sceneSerializer;
    std::string m_pendingSceneLoad;

    std::vector<MeshRenderer*>       m_meshRenderers;
    std::vector<LightComponent*>     m_lightComponents;
    std::vector<ScriptComponent*>    m_scriptComponents;

    void rebuildComponentCaches();

public:
    ModuleScene();
    ~ModuleScene();

#pragma region GameLoop
    bool init() override;
    void update() override;
    bool cleanUp() override;
#pragma endregion

#pragma region Persistence
    void saveScene();
    bool loadScene(const std::string& sceneName);

    void requestSceneChange(const std::string& sceneName);
    bool isPendingSceneLoad() const { return !m_pendingSceneLoad.empty(); }
#pragma endregion

    Scene* getScene() { return m_scene.get(); }
    Quadtree* getQuadtree() { return m_quadtree.get(); }

    const std::vector<MeshRenderer*>& getMeshRenderers();
    const std::vector<LightComponent*>& getLightComponents();
    const std::vector<ScriptComponent*>& getScriptComponents();
};