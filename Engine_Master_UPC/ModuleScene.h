#pragma once
#include "Module.h"

#include <memory>
#include <string>


class Scene;
class Quadtree;
class SceneSerializer;
class SceneSnapshot;

class GameObject;
class Component;
class CameraComponent;
class MeshRenderer;
class SpriteRenderer;
class ScriptComponent;
class LightComponent;
class IDebugDrawable;

struct ID3D12GraphicsCommandList;

class ModuleScene : public Module
{
private:
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Quadtree> m_quadtree;

    std::unique_ptr<SceneSerializer> m_sceneSerializer;
    std::string m_pendingSceneLoad;

    std::vector<MeshRenderer*>       m_meshRenderers;
    std::vector<SpriteRenderer*>     m_spriteRenderers;
    std::vector<LightComponent*>     m_lightComponents;
    std::vector<ScriptComponent*>    m_scriptComponents;

    void rebuildComponentCaches();

public:
    ModuleScene();
    ~ModuleScene();

    void rebuildMeshRenderersCache();

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

#pragma region SnapShot 
    SceneSnapshot* takeSnapshot() const;
    void loadFromSnapshot(SceneSnapshot& snapshot);
#pragma endregion


    Scene* getScene() { return m_scene.get(); }
    Quadtree* getQuadtree() { return m_quadtree.get(); }

    // This cache is not very effective, it needs to be rebuilt almost every frame (whenever any object or the camera move) if frustum culling is enabled (always in game mode)
    const std::vector<MeshRenderer*>& getMeshRenderers();
    const std::vector<SpriteRenderer*>& getSpriteRenderers();
    const std::vector<LightComponent*>& getLightComponents();
    const std::vector<ScriptComponent*>& getScriptComponents();
};