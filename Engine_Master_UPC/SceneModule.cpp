#include "Globals.h"

#include "SceneModule.h"
#include "LightComponent.h"
#include <CameraComponent.h>
#include "Application.h"
#include "RenderModule.h"
#include "EditorModule.h"
#include "Settings.h"
#include "GameObject.h"
#include "Quadtree.h"
#include "UID.h"

#include "ModelComponent.h"

#include "SceneSerializer.h"

#include <queue>

using namespace DirectX::SimpleMath;

extern Application* app;

#pragma region GameLoop
bool SceneModule::init()
{
	m_sceneSerializer = new SceneSerializer();

    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    /// PROVISIONAL
    GameObject* gameCamera = new GameObject(GenerateUID());
    gameCamera->GetTransform()->setPosition(Vector3(-5.0f, 10.0f, -5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(IM_PI / 4, IM_PI / 4, 0.0f));
    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
    app->setActiveCamera(gameCamera->GetComponentAs<CameraComponent>(ComponentType::CAMERA));
    auto component = gameCamera->GetComponentAs<ModelComponent>(ComponentType::MODEL);
    gameCamera->RemoveComponent(component);
    m_gameObjects.push_back(gameCamera);

    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->init();
    }

    auto rectangle = BoundingRect(-10, -10, 20, 20);
    m_quadtree = new Quadtree(rectangle);

    createDirectionalLightOnInit();

    applySkyboxToRenderer();

    return true;
}

void SceneModule::update()
{
    for (GameObject* root : m_gameObjects)
    {
        updateHierarchy(root);
    }
}

void SceneModule::updateHierarchy(GameObject* obj)
{
    if (!obj->GetActive())
    {
        return;
    }

    obj->update();
    for (GameObject* child : obj->GetTransform()->getAllChildren())
    {
        updateHierarchy(child);
    }

    m_quadtree->resolveDirtyNodes();
}

void SceneModule::preRender()
{
    for (GameObject* gameObject : m_gameObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->preRender();
        }
    }
}

void SceneModule::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) 
{
    CameraComponent* camera = app->getActiveCamera();

    for (GameObject* gameObject : m_gameObjects)
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

    std::vector<GameObject*> gameObjects;
    if (app->getSettings()->frustumCulling.debugFrustumCulling && camera)
    {
        gameObjects = m_quadtree->getObjects(&camera->getFrustum());
    }
    else
    {
        gameObjects = m_gameObjects;
    }

    for (GameObject* gameObject : gameObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->render(commandList, viewMatrix, projectionMatrix);
        }
    }
}

void SceneModule::postRender()
{
    for (GameObject* gameObject : m_gameObjects)
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

    delete m_quadtree;
    m_quadtree = nullptr;

    delete m_sceneSerializer;
    m_sceneSerializer = nullptr;
	return true;
}
#pragma endregion

void SceneModule::createGameObject()
{
	GameObject* newGameObject = new GameObject(GenerateUID());
    newGameObject->init();
    newGameObject->GetTransform()->setPosition(Vector3(1.0f, 0.0f, 1.0f));

    m_gameObjects.push_back(newGameObject);

    newGameObject->onTransformChange();      
    m_quadtree->insert(*newGameObject);
}

GameObject* SceneModule::createGameObjectWithUID(UID id, UID transformUID) 
{
    GameObject* newGameObject = new GameObject(id, transformUID);
    newGameObject->init();

    m_gameObjects.push_back(newGameObject);

    newGameObject->onTransformChange();
    m_quadtree->insert(*newGameObject);

    return newGameObject;
}


void SceneModule::removeGameObject(UID uuid)
{
    GameObject* target = nullptr;

    for (GameObject* root : m_gameObjects)
    {
        if (root->GetID() == uuid)
        {
            target = root;
            break;
        }

        target = findInHierarchy(root, uuid);
        if (target) 
        {
            break;
        }

    }

    if (!target)
        return;

    destroyHierarchy(target);
}

void SceneModule::addGameObject(GameObject* gameObject) 
{
	m_gameObjects.push_back(gameObject);
}

void SceneModule::detachGameObject(GameObject* gameObject)
{
    m_gameObjects.erase(
        std::remove(m_gameObjects.begin(), m_gameObjects.end(), gameObject),
        m_gameObjects.end()
    );
}

void SceneModule::destroyGameObject(GameObject* gameObject)
{
    detachGameObject(gameObject);
    delete gameObject;
}

void SceneModule::resetGameObjects(const std::vector<GameObject*> &previousGameObjects)
{
	m_gameObjects = previousGameObjects;
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
    {
        destroyHierarchy(child);
    }

    m_quadtree->remove(*obj);

    Transform* parent = obj->GetTransform()->getRoot();

    if (parent)
        parent->removeChild(obj->GetID());
    else
        detachGameObject(obj);

    obj->cleanUp();
    delete obj;
}

GameObject* SceneModule::createDirectionalLightOnInit()
{
    GameObject* go = new GameObject(GenerateUID());
    auto component = go->GetComponentAs<ModelComponent>(ComponentType::MODEL);
    go->RemoveComponent(component);

    go->SetName("Directional Light");

    go->AddComponent(ComponentType::LIGHT);

    auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT);
    if (light)
    {
        light->setTypeDirectional();
        light->editData().common.color = Vector3::One;
        light->editData().common.intensity = 1.0f;
        light->setActive(true);
        light->sanitize();
    }

    go->GetTransform()->setRotationEuler({ 180.f, 0.f, 0.f });

    go->init();
    m_gameObjects.push_back(go);
    m_quadtree->insert(*go);

    return go;
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

        for (GameObject* gameObject : m_gameObjects)
        {
            nodes_to_visit.push(gameObject);
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

bool SceneModule::loadFromJSON(const rapidjson::Value& sceneJson) 
{
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

        detachGameObject(child);
    }

    applySkyboxToRenderer();

    return true;
}

bool SceneModule::loadSceneSkybox(const rapidjson::Value& sceneJson) 
{
    auto& skybox = getSkyboxSettings();
    const auto& skyboxJson = sceneJson["Skybox"];
    skybox.enabled = skyboxJson["Enabled"].GetBool();

    const char* pathStr = skyboxJson["Path"].GetString();
    strcpy_s(skybox.path, 260, pathStr);

    return true;
}

bool SceneModule::loadSceneLighting(const rapidjson::Value& sceneJson) 
{
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
    if (!fileExists) 
    {
        return false;
    }

    m_name = sceneName;
    return true;
}

void SceneModule::clearScene()
{
    app->getEditorModule()->setSelectedGameObject(nullptr);

    while (!m_gameObjects.empty())
    {
        destroyHierarchy(m_gameObjects.back());
    }
}
#pragma endregion

