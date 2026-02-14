#include "Globals.h"
#include "SceneModule.h"
#include <CameraComponent.h>

#include "BasicModel.h"

using namespace DirectX::SimpleMath;

#pragma region GameLoop
bool SceneModule::init()
{
    m_sceneData.lightDirection = Vector3(0.0f, -1.0f, 0.0f);
    m_sceneData.lightColor = Vector3(1.0f, 1.0f, 1.0f);
    m_sceneData.ambientColor = Vector3(0.2f, 0.2f, 0.2f);
    m_sceneData.view = Vector3(0.0f, 0.0f, -5.0f);

    /// PROVISIONAL
    GameObject* gameObject = new GameObject(rand());
    gameObject->AddComponent(ComponentType::CAMERA);
    auto component = gameObject->GetComponent<BasicModel>();
    gameObject->RemoveComponent(component);
    m_gameObjects.push_back(gameObject);

    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->init();
    }

    auto rectangle = BoundingRect(0, 0, 10, 10);
    m_quadtree = new Quadtree(rectangle);



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
    for (GameObject* gameObject : m_gameObjects)
    {
        if (gameObject->GetActive())
        {
            /// Quatree TEST
            if (gameObject->GetTransform()->isDirty())
            {
                m_quadtree->move(*gameObject);
            }
            ///
            //gameObject->render(commandList, viewMatrix, projectionMatrix);
        }
    }

    /// PROVISIONAL
    CameraComponent* camera{nullptr};
    for (GameObject* gameObject : m_gameObjects)
    {
        camera = gameObject->GetComponent<CameraComponent>();
        if (camera) break;
    }

    if (camera) 
    {
        camera->render(commandList, viewMatrix, projectionMatrix);

        auto gameObjects = m_quadtree->getObjects(camera->getFrustum());
        for (GameObject* gameObject : gameObjects)
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

