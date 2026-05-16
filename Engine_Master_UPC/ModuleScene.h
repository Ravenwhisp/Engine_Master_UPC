#pragma once
#include "Module.h"

#include "ScenePicking.h"
#include "Layer.h"

#include <memory>
#include <string>
#include <filesystem> 

class Scene;
class Quadtree;
class SceneSerializer;
class SceneSnapshot;

class GameObject;
class Component;
class CameraComponent;
class MeshRenderer;
class ScriptComponent;
class LightComponent;
class IDebugDrawable;

struct ID3D12GraphicsCommandList;

class ModuleScene : public Module
{
private:
    std::shared_ptr<Scene> m_scene;

    std::unique_ptr<Quadtree> m_staticQuadtree;
    std::unique_ptr<Quadtree> m_dynamicQuadtree;

    std::unique_ptr<SceneSerializer> m_sceneSerializer;
    std::string m_pendingSceneLoad;
    std::shared_ptr<Scene> m_pendingScene;

    std::vector<MeshRenderer*>       m_meshRenderers;
    std::vector<LightComponent*>     m_lightComponents;
    std::vector<ScriptComponent*>    m_scriptComponents;

    const std::vector<Layer> m_staticLayers = { Layer::ENVIRONMENT, Layer::NAVMESH };
    const std::vector<Layer> m_dynamicLayers = { Layer::DEFAULT, Layer::PLAYER, Layer::ENEMY, Layer::PROJECTILE, Layer::BREAKABLE, Layer::PICKUP };

    void clearComponentCaches();
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
    bool loadScene(std::shared_ptr<Scene> scene);

    void requestSceneChange(const std::string& sceneName);
    void requestSceneChange(std::shared_ptr<Scene> scene);

    bool isPendingSceneLoad() const { return !m_pendingSceneLoad.empty(); }
#pragma endregion

#pragma region SnapShot 
    SceneSnapshot* takeSnapshot() const;
    void loadFromSnapshot(SceneSnapshot& snapshot);
#pragma endregion

#pragma region Quadtree
    void syncQuadtreeWithSettings();
	Quadtree* getStaticQuadtree() { return m_staticQuadtree.get(); }
	Quadtree* getDynamicQuadtree() { return m_dynamicQuadtree.get(); }
	void moveGameObjectInQuadtrees(GameObject& gameObject);
#pragma endregion

#pragma region ObjectPicking
    std::vector<GameObjectPickHit> collectAABBHits(const Ray& worldRay);
    bool pickGameObject(const Ray& worldRay, GameObjectPickHit& outHit);
#pragma endregion

    Scene* getScene() { return m_scene.get(); }

    // This cache is not very effective, it needs to be rebuilt almost every frame (whenever any object or the camera move) if frustum culling is enabled (always in game mode)
    const std::vector<MeshRenderer*>& getMeshRenderers();
    const std::vector<MeshRenderer*> getVisibleMeshRenderers();
    const std::vector<LightComponent*>& getLightComponents();
    const std::vector<ScriptComponent*>& getScriptComponents();
};