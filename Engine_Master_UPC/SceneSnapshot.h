#pragma once

#include <memory>
#include <vector>
#include <string>

#include "UID.h"
#include "SceneReferenceResolver.h"
#include "SceneLightingSettings.h"
#include "SkyBoxSettings.h"
#include "SSAOSettings.h"
#include "AssetId.h"
#include "Scene.h"

class GameObject;
class Component;
class CameraComponent;

class SceneSnapshot
{
private:
    SceneReferenceResolver m_resolver;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
    CameraComponent* m_defaultCamera = nullptr;

    SceneLightingSettings m_lighting;
    SkyBoxSettings m_skybox;
    SSAOSettings m_ssao;
    AssetId m_navMesh;
    std::vector<AssetId> m_loadedBankRefs;
    mutable std::vector<std::string> m_loadedBankNameCache;

public:
    SceneSnapshot();
    ~SceneSnapshot();

    void init(const Scene& scene);
    void applyTo(Scene& scene);

private:
    std::unique_ptr<GameObject> cloneRecursive(GameObject* original);
    void registerDescendants(GameObject* origParent, GameObject* cloneParent);
    void fixReferences();
};