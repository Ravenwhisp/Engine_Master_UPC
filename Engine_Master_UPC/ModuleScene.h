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

struct ID3D12GraphicsCommandList;

class ModuleScene : public Module
{
private:
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Quadtree> m_quadtree;

    std::unique_ptr<SceneSerializer> m_sceneSerializer;
    std::string m_pendingSceneLoad;

    std::vector<MeshRenderer*> m_meshRenderers;

public:
    ModuleScene();
    ~ModuleScene();

#pragma region GameLoop
    bool init() override;
    void update() override;
    void render(ID3D12GraphicsCommandList* commandList);
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

    std::vector<MeshRenderer*> getAllMeshRenderers() const;
};