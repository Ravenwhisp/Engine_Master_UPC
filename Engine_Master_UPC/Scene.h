#pragma once
#include "Asset.h"
#include <vector>
#include <memory>
#include <string>

#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "SkyBoxSettings.h"
#include "SoundBanksData.h"
#include "SceneReferenceResolver.h"
#include "UID.h"

#include <unordered_map>
#include <unordered_set>

struct ID3D12GraphicsCommandList;

class GameObject;
class CameraComponent;
class MeshRenderer;
class TriggerSystem;
class TriggerComponent;

class Scene: public Asset
{
    friend class SceneSnapshot;
private:
    std::string m_name = "SampleScene";

    std::vector<std::unique_ptr<GameObject>>        m_allObjects;

    SceneLightingSettings                           m_lighting;
    SceneDataCB                                     m_sceneDataCB;
    SkyBoxSettings                                  m_skybox;

    CameraComponent*                                m_defaultCamera = nullptr;

    std::unordered_set<UID>                         m_objectsToRemove;

    std::unordered_map<GameObject*, size_t>         m_objectIndexMap;
    mutable std::unordered_map<UID, GameObject*>    m_uidIndex;

    bool                                            m_componentCacheDirty = true;
    mutable bool                                    m_hierarchyCacheDirty = true;
    mutable std::vector<GameObject*>                m_allGameObjectsCache;
    mutable std::vector<GameObject*>                m_rootObjectsCache;

    std::unique_ptr<TriggerSystem>                  m_triggerSystem;

    std::vector<std::string>                        m_loadedBanks;

    struct PendingDestroyedGameObject
    {
        std::unique_ptr<GameObject> gameObject;
        uint64_t fenceValue = 0;
    };

    std::vector<PendingDestroyedGameObject>         m_pendingDestroyedObjects;

    void releasePendingDestroyedGameObjects();
    void removePendingGameObjects();
    void fixReferencesFor(const std::vector<GameObject*>& gos);

    bool isDescendant(GameObject* root, GameObject* candidate) const;

    void commitGameObject(std::unique_ptr<GameObject> gameObject);
    void ensureIndicesFresh() const;

public:
    friend class ModuleScene;

    Scene(AssetReference& uid);
    ~Scene();

    void serialize(IArchive& archive) override;
    void fixReferences();

#pragma region GameLoop

    bool init();
    void update();
    bool cleanUp();

#pragma endregion

    const char* getName() const                                 { return m_name.c_str(); }
    void setName(const char* newName)                           { m_name = newName; }

    SceneLightingSettings& getLightingSettings()                { return m_lighting; }
    const SceneLightingSettings& getLightingSettings() const    { return m_lighting; }
    SceneDataCB& getCBData()                                    { return m_sceneDataCB; }
    const SceneDataCB& getCBData() const                        { return m_sceneDataCB; }
    SkyBoxSettings& getSkyBoxSettings()                         { return m_skybox; }
    const SkyBoxSettings& getSkyBoxSettings() const             { return m_skybox; }

    CameraComponent* getDefaultCamera() const                   { return m_defaultCamera; }
    void setDefaultCamera(CameraComponent* camera)              { m_defaultCamera = camera; }

    GameObject* createGameObject();
    GameObject* createGameObjectWithUID(UID id, UID transformUID);
    void initLoadedObjects();

    GameObject* findGameObjectByUID(UID uuid);
    void removeGameObject(UID uuid);
    void markGameObjectForRemoval(UID uuid);

    void addGameObject(std::unique_ptr<GameObject> gameObject);
    void destroyGameObject(GameObject* gameObject);

    const std::vector<GameObject*>& getRootObjects() const;

    GameObject* createDirectionalLightOnInit();

    const std::vector<GameObject*>& getAllGameObjects() const;
    bool containsGameObject(const GameObject* go) const;

    void clearScene();

    void markDirty();
    bool isComponentCacheDirty() const { return m_componentCacheDirty; }
    void clearDirty() { m_componentCacheDirty = false; }

#pragma region Triggers
    void registerTrigger(TriggerComponent* trigger);
    void unregisterTrigger(TriggerComponent* trigger);
    void clearTriggers();
#pragma endregion

#pragma region MusicBanks
    const std::vector<std::string>& getLoadedBanks() const;
    void addLoadedBank(const std::string& bank);
    void removeLoadedBank(const std::string& bank);
	void unloadSoundBanks();
#pragma endregion
};