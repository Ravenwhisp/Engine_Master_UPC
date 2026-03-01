#include "Globals.h"

#include "SceneModule.h"
#include "LightComponent.h"
#include <CameraComponent.h>
#include "Application.h"
#include "RenderModule.h"
#include "EditorModule.h"
#include "Settings.h"

#include "Quadtree.h"
#include "ModelComponent.h"
#include "SceneSerializer.h"

#include <queue>
#include <limits>

using namespace DirectX::SimpleMath;

SceneModule::SceneModule() = default;
SceneModule::~SceneModule() = default;

#pragma region GameLoop
bool SceneModule::init()
{
	m_sceneSerializer = std::make_unique<SceneSerializer>();

    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    /// PROVISIONAL
    auto gameCamera = std::make_unique<GameObject>(GenerateUID());
    gameCamera->GetTransform()->setPosition(Vector3(-5.0f, 10.0f, -5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(IM_PI / 4, IM_PI / 4, 0.0f));
    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
    app->setActiveCamera(gameCamera->GetComponentAs<CameraComponent>(ComponentType::CAMERA));
    auto component = gameCamera->GetComponentAs<ModelComponent>(ComponentType::MODEL);
    gameCamera->RemoveComponent(component);
    m_allObjects.push_back(std::move(gameCamera));
	m_rootObjects.push_back(m_allObjects.back().get());

    for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
    {
        gameObject->init();
    }

    m_quadtree.reset();

    createDirectionalLightOnInit();

    applySkyboxToRenderer();

    return true;
}

void SceneModule::update()
{
    for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->update();
		}
    }

    if (m_quadtree)
    {
        m_quadtree->resolveDirtyNodes();
    }
}

void SceneModule::preRender()
{
    for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->preRender();
        }
    }
}


void SceneModule::createQuadtree()
{
    float minX = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const std::unique_ptr<GameObject>& go : m_allObjects)
    {
        Component* component = go->GetComponent(ComponentType::MODEL);
        if (component)
        {
            ModelComponent* model = static_cast<ModelComponent*>(component);
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

    auto rectangle = BoundingRect(minX, minZ, maxX-minX, maxZ-minZ);
    m_quadtree = std::make_unique<Quadtree>(rectangle);

    for (const std::unique_ptr<GameObject>& go : m_allObjects)
    {
        m_quadtree->insert((*go));
    }
    DEBUG_LOG("QUADTREE created");
}

void SceneModule::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) 
{
    CameraComponent* camera = app->getActiveCamera();

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

    const bool useCulling = app->getSettings()->frustumCulling.debugFrustumCulling && camera;

    if (useCulling)
    {
        if (!m_quadtree)
        {
            createQuadtree();
        }

        auto visibleObjects = m_quadtree->getObjects(&camera->getFrustum());

        for (GameObject* gameObject : visibleObjects)
        {
            if (gameObject->GetActive())
            {
                gameObject->render(commandList, viewMatrix, projectionMatrix);
            }
        }
    }
    else
    {
        if (m_quadtree)
        {
            m_quadtree.reset();
            DEBUG_LOG("QUADTREE removed");
        }

        for (const auto& gameObject : m_allObjects)
        {
            if (gameObject->GetActive())
            {
                gameObject->render(commandList, viewMatrix, projectionMatrix);
            }
        }
    }
}

void SceneModule::postRender()
{
    for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->postRender();
        }
    }
}

bool SceneModule::cleanUp()
{
    clearScene();

    m_quadtree.reset();

    m_sceneSerializer.reset();
	return true;
}
#pragma endregion

void SceneModule::createGameObject()
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

GameObject* SceneModule::createGameObjectWithUID(UID id, UID transformUID)
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

void SceneModule::removeGameObject(UID uuid)
{
    GameObject* target = nullptr;

    for (const auto& root : m_allObjects)
    {
        if (root->GetID() == uuid)
        {
            target = root.get();
            break;
        }

        target = findInHierarchy(root.get(), uuid);
        if (target)
        {
            break;
        }
    }

    if (!target)
    {
        return;
    }

    destroyHierarchy(target);
}

void SceneModule::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_allObjects.push_back(std::move(gameObject));
}

void SceneModule::destroyGameObject(GameObject* gameObject)
{
    removeFromRootList(gameObject);
    auto it = std::remove_if(
        m_allObjects.begin(),
        m_allObjects.end(),
        [gameObject](const std::unique_ptr<GameObject>& ptr)
        {
            return ptr.get() == gameObject;
        });

    m_allObjects.erase(it, m_allObjects.end());
}

GameObject* SceneModule::findInHierarchy(GameObject* current, UID uuid)
{
    for (GameObject* child : current->GetTransform()->getAllChildren())
    {
        if (child->GetID() == uuid)
            return child;

        GameObject* found = findInHierarchy(child, uuid);
        if (found)
            return found;
    }

    return nullptr;
}

void SceneModule::destroyHierarchy(GameObject* obj)
{
    auto children = obj->GetTransform()->getAllChildren();

    for (GameObject* child : children)
        destroyHierarchy(child);

    if (m_quadtree)
        m_quadtree->remove(*obj);

    Transform* parent = obj->GetTransform()->getRoot();

    if (parent)
        parent->removeChild(obj->GetID());

    destroyGameObject(obj);
}

GameObject* SceneModule::createDirectionalLightOnInit()
{
    auto go = std::make_unique<GameObject>(GenerateUID());
    GameObject* raw = go.get();

    auto component = raw->GetComponentAs<ModelComponent>(ComponentType::MODEL);
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

bool SceneModule::applySkyboxToRenderer()
{
    return app->getRenderModule()->applySkyboxSettings(m_skybox.enabled, m_skybox.path);
}

#pragma region Persistence
rapidjson::Value SceneModule::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value sceneInfo(rapidjson::kObjectType);

    sceneInfo.AddMember("Skybox", getSkyboxJSON(domTree), domTree.GetAllocator());
    sceneInfo.AddMember("Lighting", getLightingJSON(domTree), domTree.GetAllocator());


    // GameObjects serialization //
    {
        rapidjson::Value gameObjectsData(rapidjson::kArrayType);

        std::queue<GameObject*> nodes_to_visit;

        for (const std::unique_ptr<GameObject>& gameObject : m_allObjects)
        {
            nodes_to_visit.push(gameObject.get());
        }

        while (!nodes_to_visit.empty()) 
        {
            GameObject* gameObject = nodes_to_visit.front();
            nodes_to_visit.pop();

            gameObjectsData.PushBack(gameObject->getJSON(domTree), domTree.GetAllocator());

            for (GameObject* child : gameObject->GetTransform()->getAllChildren()) {
                nodes_to_visit.push(child);
            }
        }

        sceneInfo.AddMember("GameObjects", gameObjectsData, domTree.GetAllocator());
    }

    return sceneInfo;
}

rapidjson::Value SceneModule::getLightingJSON(rapidjson::Document& domTree)
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

rapidjson::Value SceneModule::getSkyboxJSON(rapidjson::Document& domTree)
{
    rapidjson::Value skyboxInfo(rapidjson::kObjectType);

    skyboxInfo.AddMember("Enabled", m_skybox.enabled, domTree.GetAllocator());
    {
        rapidjson::Value path(m_skybox.path, domTree.GetAllocator()); // copy char[] path
        skyboxInfo.AddMember("Path", path, domTree.GetAllocator());
    }

    return skyboxInfo;
}

bool SceneModule::loadFromJSON(const rapidjson::Value& sceneJson) {
    const auto& gameObjectsArray = sceneJson["GameObjects"].GetArray();

    clearScene();

    loadSceneSkybox(sceneJson);
    loadSceneLighting(sceneJson);

    // Create all objects and components
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::unordered_map<uint64_t, uint64_t> childToParent;

    for (auto& gameObjectJson : gameObjectsArray)
    {
        const uint64_t uid = gameObjectJson["UID"].GetUint64();
        const uint64_t transformUid = gameObjectJson["Transform"]["UID"].GetUint64();
        GameObject* gameObject = createGameObjectWithUID((UID)uid, (UID)transformUid);

        uint64_t parentUid = 0;
        gameObject->deserializeJSON(gameObjectJson, parentUid);

        uidToGo[uid] = gameObject;
        childToParent[uid] = parentUid;
    }

    // Parent Child linking
    for (const auto& childAndParent : childToParent)
    {
        const uint64_t childUid = childAndParent.first;
        const uint64_t parentUid = childAndParent.second;

        if (parentUid == 0) {
            continue;
        }

        GameObject* child = uidToGo[childUid];
        GameObject* parent = uidToGo[parentUid];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        removeFromRootList(child);
    }

    applySkyboxToRenderer();

    return true;
}

bool SceneModule::loadSceneSkybox(const rapidjson::Value& sceneJson) {
    auto& skybox = getSkyboxSettings();
    const auto& skyboxJson = sceneJson["Skybox"];
    skybox.enabled = skyboxJson["Enabled"].GetBool();

    const char* pathStr = skyboxJson["Path"].GetString();
    strcpy_s(skybox.path, 260, pathStr);

    return true;
}

bool SceneModule::loadSceneLighting(const rapidjson::Value& sceneJson) {
    auto& lighting = GetLightingSettings();
    const auto& lightingJson = sceneJson["Lighting"];

    const auto& color = lightingJson["AmbientColor"].GetArray();
    lighting.ambientColor = Vector3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());

    lighting.ambientIntensity = lightingJson["AmbientIntensity"].GetFloat();
    return true;
}

void SceneModule::saveScene()
{
    rapidjson::Document domTree;
    domTree.SetObject();

    {
        rapidjson::Value name(m_name.c_str(), domTree.GetAllocator()); // copy string m_name
        domTree.AddMember(name, getJSON(domTree), domTree.GetAllocator());
    }

	m_sceneSerializer->SaveScene(m_name, domTree);
}

bool SceneModule::loadScene(const std::string& sceneName)
{
    const bool fileExists = m_sceneSerializer->LoadScene(sceneName);
    if (!fileExists) {
        return false;
    }

    m_name = sceneName;
    return true;
}

void SceneModule::clearScene()
{
    m_rootObjects.clear();
    m_allObjects.clear();
}
#pragma endregion

std::vector<GameObject*> SceneModule::getAllGameObjects()
{
    std::vector<GameObject*> result;
    result.reserve(m_allObjects.size());

    for (const auto& obj : m_allObjects)
        result.push_back(obj.get());

    return result;
}

void SceneModule::removeFromRootList(GameObject* obj)
{
    auto it = std::remove(
        m_rootObjects.begin(),
        m_rootObjects.end(),
        obj);

    m_rootObjects.erase(it, m_rootObjects.end());
}

void SceneModule::addToRootList(GameObject* gameObject)
{
    if (!gameObject) return;

    if (std::find(m_rootObjects.begin(), m_rootObjects.end(), gameObject) == m_rootObjects.end())
    {
        m_rootObjects.push_back(gameObject);
    }
}

const std::vector<GameObject*>& SceneModule::getRootObjects() const
{
    return m_rootObjects;
}
