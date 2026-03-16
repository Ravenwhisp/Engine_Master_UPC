#include "Globals.h"
#include "Scene.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Quadtree.h"
#include "SceneSnapshot.h"


#include <limits>
#include <algorithm>

Scene::Scene() = default;
Scene::~Scene() = default;

#pragma region GameLoop

bool Scene::init()
{
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

void Scene::update()
{
    if (m_quadtree)
    {
        m_quadtree->resolveDirtyNodes();
    }

    if (app->getCurrentEngineState() != ENGINE_STATE::PLAYING)
    {
        return;
    }

    for (GameObject* root : m_rootObjects)
    {
        if (root && root->GetActive())
        {
            root->update();
        }
    }
}

void Scene::render(ID3D12GraphicsCommandList* commandList)
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
        if (m_quadtree)
        {
            m_quadtree.reset();
            DEBUG_LOG("QUADTREE removed");
        }
    }
#endif // GAME_RELEASE

}

bool Scene::cleanUp()
{
    clearScene();
    return true;
}

#pragma endregion

void Scene::createQuadtree()
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

void Scene::resetQuadtree()
{
    m_quadtree.reset();
}

#pragma region CRUD
void Scene::createGameObject()
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

GameObject* Scene::createGameObjectWithUID(UID id, UID transformUID)
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

GameObject* Scene::findGameObjectByUID(UID uuid)
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

void Scene::removeGameObject(UID uuid)
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

void Scene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_allObjects.push_back(std::move(gameObject));
}

void Scene::destroyGameObject(GameObject* gameObject)
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

void Scene::resetGameObjects(SceneSnapshot previousScene)
{
    m_allObjects = std::move(previousScene.allObjects);
    m_rootObjects = std::move(previousScene.rootObjects);
    m_defaultCamera = previousScene.defaultCamera;

    //guarrada historica a continuacion
    app->getModuleEditor()->setSelectedGameObject(nullptr);
}

GameObject* Scene::findInWindowHierarchy(GameObject* current, UID uuid)
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

void Scene::destroyWindowHierarchy(GameObject* obj)
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

GameObject* Scene::createDirectionalLightOnInit()
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

bool Scene::applySkyBoxToRenderer()
{
    return app->getModuleRender()->applySkyBoxSettings(m_skybox);
}

#pragma endregion

std::vector<GameObject*> Scene::getAllGameObjects()
{
    std::vector<GameObject*> result;
    result.reserve(m_allObjects.size());

    for (const auto& obj : m_allObjects)
    {
        result.push_back(obj.get());
    }

    return result;
}

SceneSnapshot Scene::getClonedGameObjects()
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

std::unique_ptr<GameObject> Scene::cloneGameObjectRecursive(GameObject* original, SceneSnapshot& snapshot)
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

void Scene::fixClonedReferences(const SceneSnapshot& snapshot)
{
    for (const auto& obj : snapshot.allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            component->fixReferences(snapshot.componentMap);
        }
    }
}

void Scene::removeFromRootList(GameObject* obj)
{
    auto it = std::remove(
        m_rootObjects.begin(),
        m_rootObjects.end(),
        obj);

    m_rootObjects.erase(it, m_rootObjects.end());
}

void Scene::addToRootList(GameObject* gameObject)
{
    if (!gameObject) return;

    if (std::find(m_rootObjects.begin(), m_rootObjects.end(), gameObject) == m_rootObjects.end())
    {
        m_rootObjects.push_back(gameObject);
    }
}

const std::vector<GameObject*>& Scene::getRootObjects() const
{
    return m_rootObjects;
}

void Scene::clearScene()
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
