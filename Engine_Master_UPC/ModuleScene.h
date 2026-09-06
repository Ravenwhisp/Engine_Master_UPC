#pragma once
#include "Module.h"

#include "ScenePicking.h"
#include "Layer.h"
#include "MeshRenderer.h"
#include "ComponentType.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem> 

class Scene;
class Quadtree;
class SceneSnapshot;
struct AssetId;

class GameObject;
class Component;
class CameraComponent;
class MeshRenderer;
class ScriptComponent;
class LightComponent;
class IDebugDrawable;
class ParticleSystemComponent;
class TrailComponent;
class LineRendererComponent;
class OcclusionTargetComponent;
class OcclusionOccluderComponent;

namespace Engine
{
    struct Frustum;
}

struct ID3D12GraphicsCommandList;

class ModuleScene : public Module
{
public:
    static constexpr size_t COMPONENT_TYPE_COUNT = static_cast<size_t>(ComponentType::COUNT);

    struct DetailedUpdateTimings
    {
        float releaseDestroyedMs = 0.0f;
        float removePendingMs = 0.0f;
        float gameObjectsUpdateMs = 0.0f;
        float gameObjectsLateUpdateMs = 0.0f;
        float triggerSystemMs = 0.0f;
        float flushPendingMs = 0.0f;
        float staticQuadtreeUpdateMs = 0.0f;
        float dynamicQuadtreeUpdateMs = 0.0f;
        float staticQuadtreeMoveMs = 0.0f;
        float dynamicQuadtreeMoveMs = 0.0f;
        float staticQuadtreeQueryMs = 0.0f;
        float dynamicQuadtreeQueryMs = 0.0f;
        float quadtreeAreaQueryMs = 0.0f;

        uint32_t gameObjectUpdateCalls = 0;
        uint32_t gameObjectLateUpdateCalls = 0;
        uint32_t releasedDestroyedObjects = 0;
        uint32_t pendingRemovalRequests = 0;
        uint32_t pendingAdditions = 0;
        uint32_t staticQuadtreeDirtyNodes = 0;
        uint32_t dynamicQuadtreeDirtyNodes = 0;
        uint32_t staticQuadtreeMoveCalls = 0;
        uint32_t dynamicQuadtreeMoveCalls = 0;
        uint32_t staticQuadtreeQueryCalls = 0;
        uint32_t dynamicQuadtreeQueryCalls = 0;
        uint32_t staticQuadtreeQueryResults = 0;
        uint32_t dynamicQuadtreeQueryResults = 0;
        uint32_t quadtreeAreaQueryCalls = 0;
        uint32_t quadtreeAreaQueryResults = 0;

        std::array<float, COMPONENT_TYPE_COUNT> componentUpdateMs{};
        std::array<float, COMPONENT_TYPE_COUNT> componentLateUpdateMs{};
        std::array<uint32_t, COMPONENT_TYPE_COUNT> componentUpdateCalls{};
        std::array<uint32_t, COMPONENT_TYPE_COUNT> componentLateUpdateCalls{};
    };

    struct ScriptTiming
    {
        std::string scriptName;
        std::string maxGameObjectName;
        uint64_t maxGameObjectId = 0;
        float totalMs = 0.0f;
        float maxMs = 0.0f;
        uint32_t calls = 0;
    };

    struct ScriptScopeTiming
    {
        std::string scriptName;
        std::string scopeName;
        std::string maxGameObjectName;
        uint64_t maxGameObjectId = 0;
        float totalMs = 0.0f;
        float maxMs = 0.0f;
        uint32_t calls = 0;
    };

private:
    friend class Scene;
    friend class GameObject;
    friend class Quadtree;

    std::shared_ptr<Scene> m_scene;

    std::unique_ptr<Quadtree> m_staticQuadtree;
    std::unique_ptr<Quadtree> m_dynamicQuadtree;

    std::string m_pendingSceneLoad;
    std::shared_ptr<Scene> m_pendingScene;
    AssetId m_pendingSceneAssetId;

    // Scene name -> libId translations exported in build.cfg. Populated in
    // GAME_RELEASE so loadScene(std::string) can resolve names against the
    // Library instead of the (missing) Assets/Scenes folder.
    std::unordered_map<std::string, std::string> m_buildSceneLibIds;

    std::vector<MeshRenderer*>            m_meshRenderers;
    std::vector<LightComponent*>          m_lightComponents;
    std::vector<ScriptComponent*>         m_scriptComponents;
    std::vector<ParticleSystemComponent*> m_particleSystemComponents;
    std::vector<TrailComponent*>          m_trailComponents;
    std::vector<LineRendererComponent*>   m_lineRendererComponents;
    std::vector<OcclusionTargetComponent*> m_occlusionTargetComponents;
    std::vector<OcclusionOccluderComponent*> m_occlusionOccluderComponents;

    const std::vector<Layer> m_staticLayers = { Layer::ENVIRONMENT, Layer::NAVMESH };
    const std::vector<Layer> m_dynamicLayers = { Layer::DEFAULT, Layer::PLAYER, Layer::ENEMY, Layer::PROJECTILE, Layer::BREAKABLE, Layer::PICKUP };

    bool m_detailedProfilingEnabled = false;
    DetailedUpdateTimings m_detailedUpdateTimings{};

    bool m_scriptProfilingEnabled = false;
    float m_scriptSpikeThresholdMs = 5.0f;
    float m_currentScriptTotalMs = 0.0f;
    float m_lastSpikeScriptTotalMs = 0.0f;
    uint64_t m_scriptProfilerFrame = 0;
    uint64_t m_lastSpikeFrame = 0;
    std::unordered_map<std::string, ScriptTiming> m_currentScriptTimingMap;
    std::unordered_map<std::string, ScriptScopeTiming> m_currentScriptScopeTimingMap;
    std::vector<ScriptTiming> m_currentScriptTimings;
    std::vector<ScriptTiming> m_lastSpikeScriptTimings;
    std::vector<ScriptScopeTiming> m_currentScriptScopeTimings;
    std::vector<ScriptScopeTiming> m_lastSpikeScriptScopeTimings;

    void clearComponentCaches();
    void rebuildComponentCaches();

public:
    ModuleScene();
    ~ModuleScene();

#pragma region GameLoop
    bool init() override;
    void update() override;
    bool cleanUp() override;
    void beginDetailedProfilingFrame(bool enabled);
    bool isDetailedProfilingEnabled() const { return m_detailedProfilingEnabled; }
    const DetailedUpdateTimings& getDetailedUpdateTimings() const { return m_detailedUpdateTimings; }
    void beginScriptProfilingFrame(bool enabled, float spikeThresholdMs);
    void endScriptProfilingFrame();
    void recordScriptTiming(const std::string& scriptName, const std::string& gameObjectName,
        uint64_t gameObjectId, float cpuMs);
    void recordScriptScopeTiming(const std::string& scriptName, const std::string& scopeName,
        const std::string& gameObjectName, uint64_t gameObjectId, float cpuMs);
    bool isScriptProfilingEnabled() const { return m_scriptProfilingEnabled; }
    float getCurrentScriptTotalMs() const { return m_currentScriptTotalMs; }
    float getLastSpikeScriptTotalMs() const { return m_lastSpikeScriptTotalMs; }
    float getScriptSpikeThresholdMs() const { return m_scriptSpikeThresholdMs; }
    uint64_t getScriptProfilerFrame() const { return m_scriptProfilerFrame; }
    uint64_t getLastSpikeFrame() const { return m_lastSpikeFrame; }
    const std::vector<ScriptTiming>& getCurrentScriptTimings() const { return m_currentScriptTimings; }
    const std::vector<ScriptTiming>& getLastSpikeScriptTimings() const { return m_lastSpikeScriptTimings; }
    const std::vector<ScriptScopeTiming>& getCurrentScriptScopeTimings() const { return m_currentScriptScopeTimings; }
    const std::vector<ScriptScopeTiming>& getLastSpikeScriptScopeTimings() const { return m_lastSpikeScriptScopeTimings; }
#pragma endregion

#pragma region Persistence
    void saveScene();
    bool loadScene(const std::string& sceneName);
    bool loadScene(std::shared_ptr<Scene> scene);
    bool loadScene(const AssetId& ref);

    void requestSceneChange(const std::string& sceneName);
    void requestSceneChange(std::shared_ptr<Scene> scene);
    void requestSceneChange(const AssetId& ref);

    void setBuildSceneLibIds(std::unordered_map<std::string, std::string> map);

    bool isPendingSceneLoad() const { return !m_pendingSceneLoad.empty(); }

    void onGameStop();
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
    void removeGameObjectFromQuadtree(GameObject& gameObject);
#pragma endregion

#pragma region ObjectPicking
    std::vector<GameObjectPickHit> collectAABBHits(const Ray& worldRay);
    bool pickGameObject(const Ray& worldRay, GameObjectPickHit& outHit);
#pragma endregion

#pragma region Systems
    void initializeRuntimeSceneSystems();
    void clearRuntimeSceneSystems();

    // Removes raw component pointers before Scene starts deferred cleanup.
    void invalidateComponentCaches();
#pragma endregion

    Scene* getScene() { return m_scene.get(); }

    // This cache is not very effective, it needs to be rebuilt almost every frame (whenever any object or the camera move) if frustum culling is enabled (always in game mode)
    const std::vector<MeshRenderer*>& getMeshRenderers();
    const std::vector<MeshRenderer*> getDeferredMeshRenderers();
    const std::vector<MeshRenderer*> getForwardMeshRenderers();
    const std::vector<MeshRenderer*> getForwardMeshRenderers(RenderMode mode);
    const std::vector<MeshRenderer*> getVisibleMeshRenderers();
    const std::vector<MeshRenderer*> getMeshRenderersInFrustum(const Engine::Frustum& frustum);
    const std::vector<MeshRenderer*> getVisibleDeferredMeshRenderers();
    const std::vector<MeshRenderer*> getVisibleForwardMeshRenderers();
    const std::vector<MeshRenderer*> getVisibleForwardMeshRenderers(RenderMode mode);
    const std::vector<LightComponent*>& getLightComponents();
    const std::vector<ScriptComponent*>& getScriptComponents();
    const std::vector<ParticleSystemComponent*>& getParticleSystemComponents();
    const std::vector<TrailComponent*>& getTrailComponents();
    const std::vector<LineRendererComponent*>& getLineRendererComponents();
    const std::vector<OcclusionTargetComponent*>& getOcclusionTargetComponents();
    const std::vector<OcclusionOccluderComponent*>& getOcclusionOccluderComponents();
};
