#include "Globals.h"

#include "ModuleScene.h"
#include "LightComponent.h"
#include <CameraComponent.h>
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include "Settings.h"
#include "GameObject.h"
#include "UID.h"

#include "Quadtree.h"
#include "SceneSerializer.h"
#include "ModuleNavigation.h"

#include <queue>
#include <limits>

using namespace DirectX::SimpleMath;

ModuleScene::ModuleScene() = default;
ModuleScene::~ModuleScene() = default;

#pragma region GameLoop
bool ModuleScene::init()
{
    m_sceneSerializer = std::make_unique<SceneSerializer>();

    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    /// PROVISIONAL
    auto gameCamera = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = gameCamera.get();
    gameCamera->GetTransform()->setPosition(Vector3(-5.0f, 10.0f, -5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(IM_PI / 4, IM_PI / 4, 0.0f));
    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
 
    m_allObjects.push_back(std::move(gameCamera));
    m_rootObjects.push_back(rawPtr);

    for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
    {
        gameObject->init();
    }

    m_quadtree.reset();

    createDirectionalLightOnInit();

    applySkyBoxToRenderer();

    return true;
}

void ModuleScene::update()
{
    if (!m_pendingSceneLoad.empty())
    {
        loadScene(m_pendingSceneLoad);
        m_pendingSceneLoad.clear();
    }  
  
  if (m_quadtree)
    {
        m_quadtree->resolveDirtyNodes();
    }

}


void ModuleScene::createQuadtree()
{
    float minX = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const std::unique_ptr<GameObject>& go : m_allObjects)
    {
        if (!go->GetActive())
        {
            continue;
        }

        Component* component = go->GetComponent(ComponentType::MODEL);
        if (component)
        {
            MeshRenderer* model = static_cast<MeshRenderer*>(component);
            Engine::BoundingBox boundingBox = model->getBoundingBox();

            Vector3 wmin(FLT_MAX, FLT_MAX, FLT_MAX);
            Vector3 wmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            const Vector3* pts = boundingBox.getPoints();

            for (int i = 0; i < 8; ++i)
            {
                wmin.x = std::min(wmin.x, pts[i].x);
                wmin.z = std::min(wmin.z, pts[i].z);

                wmax.x = std::max(wmax.x, pts[i].x);
                wmax.z = std::max(wmax.z, pts[i].z);
            }

            minX = std::min(minX, wmin.x);
            minZ = std::min(minZ, wmin.z);
            maxX = std::max(maxX, wmax.x);
            maxZ = std::max(maxZ, wmax.z);
        }
    }
    minX -= app->getSettings()->frustumCulling.quadtreeXExtraSize;
    minZ -= app->getSettings()->frustumCulling.quadtreeZExtraSize;
    maxX += app->getSettings()->frustumCulling.quadtreeXExtraSize;
    maxZ += app->getSettings()->frustumCulling.quadtreeZExtraSize;

    auto rectangle = BoundingRect(minX, minZ, maxX - minX, maxZ - minZ);
    m_quadtree = std::make_unique<Quadtree>(rectangle);

    for (const std::unique_ptr<GameObject>& go : m_allObjects)
    {
        m_quadtree->insert((*go));
    }

    DEBUG_LOG("QUADTREE created");
}

void ModuleScene::render(ID3D12GraphicsCommandList* commandList) 
{
    if (m_quadtree)
    {
        for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
        {
            if (!gameObject->GetActive())
            {
                continue;
            }

            if (gameObject->GetTransform()->isDirty())
            {
                m_quadtree->move(*gameObject);
            }
        }
    }

#ifdef GAME_RELEASE
    const bool useCulling = app->getSettings()->frustumCulling.debugFrustumCulling;

    if (!m_quadtree)
    {
        createQuadtree();
    }

    m_meshRenderers.clear();
    if (useCulling)
    {
        auto visibleObjects = m_quadtree->getObjects(&m_defaultCamera->getFrustum());

        for (GameObject* gameObject : visibleObjects)
        {
            if (gameObject->GetActive())
            {
                auto meshRenderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
                if (meshRenderer && meshRenderer->hasMeshes())
                {
                    m_meshRenderers.push_back(meshRenderer);
                }
            }
        }
    }
    else
    {
        for (GameObject* gameObject : getAllGameObjects())
        {
            if (gameObject->GetActive())
            {
                auto meshRenderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
                if (meshRenderer && meshRenderer->hasMeshes())
                {
                    m_meshRenderers.push_back(meshRenderer);
                }
            }
        }
    }


#else
    const bool useCulling = app->getSettings()->frustumCulling.debugFrustumCulling && m_defaultCamera;

    m_meshRenderers.clear();
    if (useCulling)
    {
        if (!m_quadtree)
        {
            createQuadtree();
        }

        auto visibleObjects = m_quadtree->getObjects(&m_defaultCamera->getFrustum());

        for (GameObject* gameObject : visibleObjects)
        {
            if (gameObject->GetActive())
            {
                auto meshRenderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
                if (meshRenderer)
                {
                    m_meshRenderers.push_back(meshRenderer);
                }
            }
        }
    }
    else
    {		
        for (GameObject* gameObject : getAllGameObjects())
        {
            if (gameObject->GetActive())
            {
                auto meshRenderer = gameObject->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
                if (meshRenderer)
                {
                    m_meshRenderers.push_back(meshRenderer);
                }
            }
        }
        if (m_quadtree)
        {
            m_quadtree.reset();
            DEBUG_LOG("QUADTREE removed");
        }
    }
#endif // GAME_RELEASE

}

bool ModuleScene::cleanUp()
{
    clearScene();

    m_quadtree.reset();

    m_sceneSerializer.reset();
    return true;
}
#pragma endregion

void ModuleScene::createGameObject()
{
    std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = newGameObject.get();
    rawPtr->init();
    rawPtr->GetTransform()->setPosition(Vector3(1.0f, 0.0f, 1.0f));

    m_allObjects.push_back(std::move(newGameObject));
    m_rootObjects.push_back(rawPtr);

    rawPtr->onTransformChange();

    if (m_quadtree)
    {
        m_quadtree->insert(*rawPtr);
    }
}

GameObject* ModuleScene::createGameObjectWithUID(UID id, UID transformUID)
{
    auto newGameObject = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGameObject.get();

    raw->init();

    m_allObjects.push_back(std::move(newGameObject));
    m_rootObjects.push_back(raw);

    raw->onTransformChange();

    if (m_quadtree)
    {
        m_quadtree->insert(*raw);
    }

    return raw;
}

GameObject* ModuleScene::findGameObjectByUID(UID uuid)
{
    for (const auto& root : m_allObjects)
    {
        if (root->GetID() == uuid)
        {
            return root.get();
        }

        if (GameObject* found = findInWindowHierarchy(root.get(), uuid))
        {
            return found;
        }
    }
    return nullptr;
}

void ModuleScene::removeGameObject(UID uuid)
{
    GameObject* target = nullptr;

    for (const auto& root : m_allObjects)
    {
        if (root->GetID() == uuid)
        {
            target = root.get();
            break;
        }

        target = findInWindowHierarchy(root.get(), uuid);
        if (target)
        {
            break;
        }
    }

    if (!target)
    {
        return;
    }

    destroyWindowHierarchy(target);
}

void ModuleScene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_allObjects.push_back(std::move(gameObject));
}

void ModuleScene::destroyGameObject(GameObject* gameObject)
{
    removeFromRootList(gameObject);

    auto it = std::find_if(
        m_allObjects.begin(),
        m_allObjects.end(),
        [gameObject](const std::unique_ptr<GameObject>& ptr)
        {
            return ptr.get() == gameObject;
        });

    if (it != m_allObjects.end())
    {
        (*it)->cleanUp();
        m_allObjects.erase(it);
    }
}

void ModuleScene::resetGameObjects(SceneSnapshot previousScene)
{
    m_allObjects = std::move(previousScene.allObjects);
	m_rootObjects = std::move(previousScene.rootObjects);
	m_defaultCamera = previousScene.defaultCamera;

    //guarrada historica a continuacion
    app->getModuleEditor()->setSelectedGameObject(nullptr);
}

GameObject* ModuleScene::findInWindowHierarchy(GameObject* current, UID uuid)
{
    for (GameObject* child : current->GetTransform()->getAllChildren())
    {
        if (child->GetID() == uuid)
        {
            return child;
        }

        GameObject* found = findInWindowHierarchy(child, uuid);
        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

void ModuleScene::destroyWindowHierarchy(GameObject* obj)
{
    auto children = obj->GetTransform()->getAllChildren();

    for (GameObject* child : children)
    {
        destroyWindowHierarchy(child);
    }

    if (m_quadtree)
    {
        m_quadtree->remove(*obj);
    }

    Transform* parent = obj->GetTransform()->getRoot();

    if (parent)
    {
        parent->removeChild(obj->GetID());
    }

    destroyGameObject(obj);
}

GameObject* ModuleScene::createDirectionalLightOnInit()
{
    auto go = std::make_unique<GameObject>(GenerateUID());
    GameObject* raw = go.get();

    auto component = raw->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    raw->RemoveComponent(component);

    raw->SetName("Directional Light");
    raw->AddComponent(ComponentType::LIGHT);

    auto* light = raw->GetComponentAs<LightComponent>(ComponentType::LIGHT);
    if (light)
    {
        light->setTypeDirectional();
        light->editData().common.color = Vector3::One;
        light->editData().common.intensity = 1.0f;
        light->setActive(true);
        light->sanitize();
    }

    raw->GetTransform()->setRotationEuler({ 180.f, 0.f, 0.f });
    raw->init();

    m_allObjects.push_back(std::move(go));
    m_rootObjects.push_back(raw);

    if (m_quadtree)
    {
        m_quadtree->insert(*raw);
    }

    return raw;
}

bool ModuleScene::applySkyBoxToRenderer()
{
    return app->getModuleRender()->applySkyBoxSettings(m_skybox);
}


#pragma region Persistence
rapidjson::Value ModuleScene::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value sceneInfo(rapidjson::kObjectType);

    sceneInfo.AddMember("SkyBox", getSkyBoxJSON(domTree), domTree.GetAllocator());
    sceneInfo.AddMember("Lighting", getLightingJSON(domTree), domTree.GetAllocator());

    uint64_t defaultCameraOwnerUid = 0;
    if (m_defaultCamera != nullptr) 
    {
        GameObject* owner = m_defaultCamera->getOwner();
        defaultCameraOwnerUid = (uint64_t)owner->GetID();
    }

    sceneInfo.AddMember("DefaultCameraOwnerUID", defaultCameraOwnerUid, domTree.GetAllocator());

    // GameObjects serialization //
    {
        rapidjson::Value gameObjectsData(rapidjson::kArrayType);

        for (GameObject* root : m_rootObjects)
        {
            serializeWindowHierarchy(root, gameObjectsData, domTree);
        }

        sceneInfo.AddMember("GameObjects", gameObjectsData, domTree.GetAllocator());
    }

    return sceneInfo;
}

void ModuleScene::serializeWindowHierarchy(GameObject* gameObject, rapidjson::Value& gameObjectsData, rapidjson::Document& domTree)
{
    gameObjectsData.PushBack(gameObject->getJSON(domTree), domTree.GetAllocator());

    for (GameObject* child : gameObject->GetTransform()->getAllChildren())
    {
        serializeWindowHierarchy(child, gameObjectsData, domTree);
    }
}

rapidjson::Value ModuleScene::getLightingJSON(rapidjson::Document& domTree)
{
    rapidjson::Value lightingInfo(rapidjson::kObjectType);

    {
        rapidjson::Value ambientColorData(rapidjson::kArrayType);
        ambientColorData.PushBack(m_lighting.ambientColor.x, domTree.GetAllocator());
        ambientColorData.PushBack(m_lighting.ambientColor.y, domTree.GetAllocator());
        ambientColorData.PushBack(m_lighting.ambientColor.z, domTree.GetAllocator());

        lightingInfo.AddMember("AmbientColor", ambientColorData, domTree.GetAllocator());
    }

    lightingInfo.AddMember("AmbientIntensity", m_lighting.ambientIntensity, domTree.GetAllocator());

    return lightingInfo;
}

rapidjson::Value ModuleScene::getSkyBoxJSON(rapidjson::Document& domTree)
{
    rapidjson::Value skyboxInfo(rapidjson::kObjectType);

    skyboxInfo.AddMember("Enabled", m_skybox.enabled, domTree.GetAllocator());
    skyboxInfo.AddMember("CubemapAssetId", rapidjson::Value(m_skybox.cubemapAssetId.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    return skyboxInfo;
}

bool ModuleScene::loadFromJSON(const rapidjson::Value& sceneJson) 
{
    const auto& gameObjectsArray = sceneJson["GameObjects"].GetArray();

    clearScene();

    if (!loadSceneSkyBox(sceneJson))
    {
		DEBUG_LOG("Failed to load skybox settings from scene JSON. Possible wrong version of scene data.");
        return false;
    }
    loadSceneLighting(sceneJson);

    // Create all objects and components
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::vector<std::pair<uint64_t, uint64_t>> childToParent;

    for (auto& gameObjectJson : gameObjectsArray)
    {
        const uint64_t uid = gameObjectJson["UID"].GetUint64();
        const uint64_t transformUid = gameObjectJson["Transform"]["UID"].GetUint64();
        GameObject* gameObject = createGameObjectWithUID((UID)uid, (UID)transformUid);

        uint64_t parentUid = 0;
        gameObject->deserializeJSON(gameObjectJson, parentUid);

        uidToGo[uid] = gameObject;
        childToParent.push_back({ uid, parentUid });
    }

    // Parent Child linking
    for (const auto& pair : childToParent)
    {
        const uint64_t childUid = pair.first;
        const uint64_t parentUid = pair.second;

        if (parentUid == 0) {
            continue;
        }

        GameObject* child = uidToGo[childUid];
        GameObject* parent = uidToGo[parentUid];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        removeFromRootList(child);
    }

    fixLoadedSceneReferences();
    resolveDefaultCamera(sceneJson);
    applySkyBoxToRenderer();

    return true;
}

bool ModuleScene::loadSceneSkyBox(const rapidjson::Value& sceneJson)
{
    if (!sceneJson.HasMember("SkyBox"))
    {
        return false;
    }

    auto& skybox = getSkyBoxSettings();
    const auto& skyboxJson = sceneJson["SkyBox"];

    if (!skyboxJson.HasMember("Enabled") && skyboxJson["Enabled"].IsBool())
    {
        return false;
    }
    skybox.enabled = skyboxJson["Enabled"].GetBool();

    if (!skyboxJson.HasMember("CubemapAssetId") && skyboxJson["CubemapAssetId"].GetString())
    {
        return false;
    }
    skybox.cubemapAssetId = skyboxJson["CubemapAssetId"].GetString();

    return true;
}

bool ModuleScene::loadSceneLighting(const rapidjson::Value& sceneJson) 
{
    auto& lighting = GetLightingSettings();
    const auto& lightingJson = sceneJson["Lighting"];

    const auto& color = lightingJson["AmbientColor"].GetArray();
    lighting.ambientColor = Vector3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());

    lighting.ambientIntensity = lightingJson["AmbientIntensity"].GetFloat();
    return true;
}

void ModuleScene::fixLoadedSceneReferences()
{
    std::unordered_map<UID, Component*> componentMap;

    for (const auto& obj : m_allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            componentMap[component->getID()] = component;
        }
    }

    for (const auto& obj : m_allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            component->fixReferences(componentMap);
        }
    }
}

void ModuleScene::resolveDefaultCamera(const rapidjson::Value& sceneJson) 
{
    m_defaultCamera = nullptr;

    if (sceneJson.HasMember("DefaultCameraOwnerUID"))
    {
        const uint64_t cameraOwnerUID = sceneJson["DefaultCameraOwnerUID"].GetUint64();

        if (cameraOwnerUID != 0)
        {
            GameObject* ownerGameObject = findGameObjectByUID((UID)cameraOwnerUID);
            CameraComponent* cameraComponent = ownerGameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
            m_defaultCamera = cameraComponent;
        }
    }
}


void ModuleScene::saveScene()
{
    rapidjson::Document domTree;
    domTree.SetObject();

    {
        rapidjson::Value name(m_name.c_str(), domTree.GetAllocator()); // copy string m_name
        domTree.AddMember(name, getJSON(domTree), domTree.GetAllocator());
    }

    m_sceneSerializer->SaveScene(m_name, domTree);
}

bool ModuleScene::loadScene(const std::string& sceneName)
{
	m_quadtree.reset();
	clearScene();
    const bool fileExists = m_sceneSerializer->LoadScene(sceneName);
    if (!fileExists) 
    {
        return false;
    }

    m_name = sceneName;

    const char* s = sceneName.c_str();
    if (app->getModuleNavigation()->loadNavMeshForScene(s))
    {
        DEBUG_LOG("LOADED NavMesh for scene: %s\n", s);
    }
    else
    {
        DEBUG_ERROR("CANNOT load NavMesh for this scene\n");
    }
   
    return true;
}

void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
}

void ModuleScene::clearScene()
{
    app->getModuleEditor()->setSelectedGameObject(nullptr);

    for (auto& go : m_allObjects)
    {
        go->cleanUp();
    }

    m_rootObjects.clear();
    m_allObjects.clear();
  

    m_defaultCamera = nullptr;
}
#pragma endregion

std::vector<GameObject*> ModuleScene::getAllGameObjects()
{
    std::vector<GameObject*> result;
    result.reserve(m_allObjects.size());

    for (const auto& obj : m_allObjects)
    {
        result.push_back(obj.get());
    }

    return result;
}

SceneSnapshot ModuleScene::getClonedGameObjects()
{
	SceneSnapshot snapshot;

  /*snapshot.allObjects.reserve(m_allObjects.size());

    for (const auto& obj : m_rootObjects)
    {
        auto clone = obj->clone();

        if(find(m_rootObjects.begin(), m_rootObjects.end(), obj.get()) != m_rootObjects.end())
        {
            snapshot.rootObjects.push_back(clone.get());
		}
        if(obj->GetComponent(ComponentType::CAMERA) == m_defaultCamera)
        {
            snapshot.defaultCamera = clone->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
		}

        snapshot.allObjects.push_back(std::move(clone));
    }

    return snapshot;*/
    for (GameObject* root : m_rootObjects)
    {
        auto clonedRoot = cloneGameObjectRecursive(root, snapshot);
        snapshot.rootObjects.push_back(clonedRoot.get());
        snapshot.allObjects.push_back(std::move(clonedRoot));
    }

    fixClonedReferences(snapshot);

    return snapshot;
}

std::unique_ptr<GameObject> ModuleScene::cloneGameObjectRecursive(GameObject* original, SceneSnapshot& snapshot)
{
    auto clone = original->clone(snapshot);

    if (original->GetComponent(ComponentType::CAMERA) == m_defaultCamera)
    {
        snapshot.defaultCamera = clone->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
    }

    //clone->ClearComponents();

    Transform* originalTransform = original->GetTransform();

    for (GameObject* childGO : originalTransform->getAllChildren())
    {
        auto clonedChild = cloneGameObjectRecursive(childGO, snapshot);

        clonedChild->GetTransform()->setRoot(clone->GetTransform());
		clone->GetTransform()->addChild(clonedChild.get());
		
        snapshot.allObjects.push_back(std::move(clonedChild));
    }

    return clone;
}

void ModuleScene::fixClonedReferences(const SceneSnapshot& snapshot)
{
    for (const auto& obj : snapshot.allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
			component->fixReferences(snapshot.componentMap);
        }
    }
}

void ModuleScene::removeFromRootList(GameObject* obj)
{
    auto it = std::remove(
        m_rootObjects.begin(),
        m_rootObjects.end(),
        obj);

    m_rootObjects.erase(it, m_rootObjects.end());
}

void ModuleScene::addToRootList(GameObject* gameObject)
{
    if (!gameObject) return;

    if (std::find(m_rootObjects.begin(), m_rootObjects.end(), gameObject) == m_rootObjects.end())
    {
        m_rootObjects.push_back(gameObject);
    }
}

const std::vector<GameObject*>& ModuleScene::getRootObjects() const
{
    return m_rootObjects;
}