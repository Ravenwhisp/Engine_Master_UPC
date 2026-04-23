#pragma once
#include "ISerializable.h"
#include <vector>
#include <memory>
#include <string>

#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "SkyBoxSettings.h"
#include "UID.h"

struct ID3D12GraphicsCommandList;

class GameObject;
class CameraComponent;
class MeshRenderer;

class Scene: public ISerializable
{
    friend class SceneSnapshot;
private:
    std::string m_name = "SampleScene";

    std::vector<std::unique_ptr<GameObject>> m_allObjects;

    SceneLightingSettings m_lighting;
    SceneDataCB m_sceneDataCB;
    SkyBoxSettings m_skybox;

    CameraComponent* m_defaultCamera;
    std::vector<GameObject*> m_rootObjects;

    std::vector<UID> m_objectsToRemove;

    bool m_componentCacheDirty = true;

    void removePendingGameObjects();

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
    //

public:
    friend class ModuleScene;

    Scene();
    ~Scene();

#pragma region GameLoop

    bool init();
    void update();
    bool cleanUp();

#pragma endregion

#pragma region Persistence
    bool toJson(rapidjson::Document& domTree) const override;
    bool fromJson(const rapidjson::Value& json) override;
    AssetType getAssetType() const override { return AssetType::SCENE; }
#pragma endregion

    const char* getName() const { return m_name.c_str(); }
    void setName(const char* newName) { m_name = newName; }

    SceneLightingSettings& getLightingSettings() { return m_lighting; }
    const SceneLightingSettings& getLightingSettings() const { return m_lighting; }
    SceneDataCB& getCBData() { return m_sceneDataCB; }
    const SceneDataCB& getCBData() const { return m_sceneDataCB; }
    SkyBoxSettings& getSkyBoxSettings() { return m_skybox; }
    const SkyBoxSettings& getSkyBoxSettings() const { return m_skybox; }


    CameraComponent* getDefaultCamera() const { return m_defaultCamera; }
    void setDefaultCamera(CameraComponent* camera) { m_defaultCamera = camera; }

    GameObject* createGameObject();
    GameObject* createGameObjectWithUID(UID id, UID transformUID);
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

    void clearScene();

    void  markDirty();
    bool  isComponentCacheDirty() const { return m_componentCacheDirty; }
    void  clearDirty() { m_componentCacheDirty = false; }
};