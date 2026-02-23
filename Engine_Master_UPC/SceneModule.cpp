#include "Globals.h"
#include "SceneModule.h"
#include "LightComponent.h"
#include <CameraComponent.h>
#include "Application.h"
#include "Settings.h"

#include "BasicModel.h"

using namespace DirectX::SimpleMath;

extern Application* app;

#pragma region GameLoop
bool SceneModule::init()
{
    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    /// PROVISIONAL
    GameObject* gameCamera = new GameObject(rand());
    gameCamera->GetTransform()->setPosition(Vector3(-5.0f, 10.0f, -5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(IM_PI / 4, IM_PI / 4, 0.0f));
    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
    app->setActiveCamera(gameCamera->GetComponentAs<CameraComponent>(ComponentType::CAMERA));
    auto component = gameCamera->GetComponentAs<BasicModel>(ComponentType::MODEL);
    gameCamera->RemoveComponent(component);
    m_gameObjects.push_back(gameCamera);

    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->init();
    }

    auto rectangle = BoundingRect(-10, -10, 20, 20);
    m_quadtree = new Quadtree(rectangle);

    createDirectionalLightOnInit();

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
    CameraComponent* camera = nullptr;

    for (GameObject* gameObject : m_gameObjects)
    {
        if (!gameObject->GetActive())
            continue;

            if (gameObject->GetTransform()->isDirty())
            {
                m_quadtree->move(*gameObject);
            }

        if (!camera)
        {
            camera = gameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
        }
    }

    if (!camera) return;
    
    camera->render(commandList, viewMatrix, projectionMatrix);

    std::vector<GameObject*> gameObjects;
    if (app->getSettings()->frustumCulling.cullObjectsOutsideOfFrustum)
    {
        gameObjects = m_quadtree->getObjects(&camera->getFrustum());
    }
    else
    {
        gameObjects = m_quadtree->getObjects(nullptr);
    }

    for (GameObject* gameObject : gameObjects)
    {
        if (gameObject != camera->getOwner())
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
    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->cleanUp();
        delete gameObject;
    }
    m_gameObjects.clear();
	return true;
}
#pragma endregion

void SceneModule::createGameObject()
{
	GameObject* newGameObject = new GameObject(rand());
    newGameObject->init();
    newGameObject->GetTransform()->setPosition(Vector3(1.0f, 0.0f, 1.0f));

    m_gameObjects.push_back(newGameObject);
    m_quadtree->insert(*newGameObject);
}

void SceneModule::removeGameObject(int uuid)
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

    m_quadtree->remove(*target);
    destroyHierarchy(target);
}

void SceneModule::addGameObject(GameObject* gameObject) {
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

GameObject* SceneModule::findInHierarchy(GameObject* current, int uuid)
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
    GameObject* go = new GameObject(rand());
    auto component = go->GetComponentAs<BasicModel>(ComponentType::MODEL);
    go->RemoveComponent(component);

    go->SetName("Directional Light");

    go->AddComponent(ComponentType::LIGHT);

    auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT);
    if (light)
    {
        light->setTypeDirectional();
        light->editData().common.color = Vector3::One;
        light->editData().common.intensity = 1.0f;
        light->editData().common.enabled = true;
        light->sanitize();
    }

    go->GetTransform()->setRotationEuler({ 180.f, 0.f, 0.f });

    go->init();
    m_gameObjects.push_back(go);
    m_quadtree->insert(*go);

    return go;
}

