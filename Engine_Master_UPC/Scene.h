#pragma once
#include "Asset.h"
#include <vector>
#include <memory>
#include <string>

#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "SkyBoxSettings.h"
#include "PostProcessSettings.h"
#include "SoundBanksData.h"
#include "SceneReferenceResolver.h"
#include "UID.h"

#include <unordered_map>

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
    std::string m_rawJson;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;

    SceneLightingSettings m_lighting;
    SceneDataCB m_sceneDataCB;
    SkyBoxSettings m_skybox;
    PostProcessSettings m_postProcess;

    CameraComponent* m_defaultCamera;
    std::vector<GameObject*> m_rootObjects;

    std::vector<UID> m_objectsToRemove;

    std::unordered_map<GameObject*, size_t> m_objectIndexMap;

    bool m_componentCacheDirty = true;

    std::unique_ptr<TriggerSystem> m_triggerSystem;

    void removePendingGameObjects();

    std::vector<std::string> m_loadedBanks;

    //THIS IS A UGLY PATCH, WILL NEED A REAL REFACTOR TO SOLVE THIS PROBLEM
    bool m_isUpdating = false;

    std::vector<std::unique_ptr<GameObject>> m_pendingObjectsToAdd;
    std::vector<GameObject*> m_pendingRootObjectsToAdd;

    void flushPendingGameObjects();


    struct PendingDestroyedGameObject
    {
        std::unique_ptr<GameObject> gameObject;
        uint64_t fenceValue = 0;
    };

    std::vector<PendingDestroyedGameObject> m_pendingDestroyedObjects;
    void releasePendingDestroyedGameObjects();

    void fixReferencesFor(const std::vector<GameObject*>& gos);
    //

public:
    friend class ModuleScene;

    Scene(AssetReference& uid);
    ~Scene();

    void serialize(IArchive& archive) override;
    void FixReferences();

#pragma region GameLoop

    bool init();
    void update();
    bool cleanUp();

#pragma endregion

    const char* getName() const { return m_name.c_str(); }
    void setName(const char* newName) { m_name = newName; }

    const std::string& getRawJson() const { return m_rawJson; }
    void setRawJson(const std::string& json) { m_rawJson = json; }

    SceneLightingSettings& getLightingSettings() { return m_lighting; }
    const SceneLightingSettings& getLightingSettings() const { return m_lighting; }
    SceneDataCB& getCBData() { return m_sceneDataCB; }
    const SceneDataCB& getCBData() const { return m_sceneDataCB; }
    SkyBoxSettings& getSkyBoxSettings() { return m_skybox; }
    const SkyBoxSettings& getSkyBoxSettings() const { return m_skybox; }
    PostProcessSettings& getPostProcessSettings() { return m_postProcess; }
    const PostProcessSettings& getPostProcessSettings() const { return m_postProcess; }


    CameraComponent* getDefaultCamera() const { return m_defaultCamera; }
    void setDefaultCamera(CameraComponent* camera) { m_defaultCamera = camera; }

    GameObject* createGameObject();
    GameObject* createGameObjectWithUID(UID id, UID transformUID);
    void initLoadedObjects();

    GameObject* findGameObjectByUID(UID uuid);
    void removeGameObject(UID uuid);
    void markGameObjectForRemoval(UID uuid);

    void addGameObject(std::unique_ptr<GameObject> gameObject);
    void destroyGameObject(GameObject* gameObject);
    bool isInHierarchy(GameObject* root, GameObject* candidate) const;
    GameObject* findInWindowHierarchy(GameObject* current, UID uuid);
    void destroyWindowHierarchy(GameObject* obj);

    void addToRootList(GameObject* gameObject);
    void removeFromRootList(GameObject* gameObject);

    const std::vector<GameObject*>& getRootObjects() const;

    GameObject* createDirectionalLightOnInit();

    const std::vector<GameObject*> getAllGameObjects() const;
    bool containsGameObject(const GameObject* go) const;

    void fixSceneReferences();

    void clearScene();

    void  markDirty();
    bool  isComponentCacheDirty() const { return m_componentCacheDirty; }
    void  clearDirty() { m_componentCacheDirty = false; }

#pragma region Triggers
    void registerTrigger(TriggerComponent* trigger);
    void unregisterTrigger(TriggerComponent* trigger);
    void clearTriggers();

    void registerAllTriggersInScene();

    void registerTriggersInGameObject(GameObject* gameObject);
    void unregisterTriggersInGameObject(GameObject* gameObject);
#pragma endregion

#pragma region MusicBanks
    const std::vector<std::string>& getLoadedBanks() const;
    void addLoadedBank(const std::string& bank);
    void removeLoadedBank(const std::string& bank);
	void unloadSoundBanks();
#pragma endregion
};