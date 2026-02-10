#include "Globals.h"
#include "SceneModule.h"

using namespace DirectX::SimpleMath;

#pragma region GameLoop

bool SceneModule::init()
{
    m_sceneData.lightDirection = Vector3(0.0f, -1.0f, 0.0f);
    m_sceneData.lightColor = Vector3(1.0f, 1.0f, 1.0f);
    m_sceneData.ambientColor = Vector3(0.2f, 0.2f, 0.2f);
    m_sceneData.view = Vector3(0.0f, 0.0f, -5.0f);
    
    auto rectangle = RectangleData(0, 0, 10, 10);
    m_quadtree = new Quadtree(rectangle);

    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->update();
    }

    return true;
}

void SceneModule::update()
{
    for (GameObject* gameObject : m_gameObjects)
    {
        if (gameObject->GetActive())
        {
            gameObject->update();
		}
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

void SceneModule::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) {

    for (GameObject* gameObject : m_gameObjects)
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
    for (GameObject* gameObject : m_gameObjects)
    {
        gameObject->cleanUp();
        delete gameObject;
    }
    m_gameObjects.clear();
	return true;
}
#pragma endregion

void SceneModule::CreateGameObject()
{
    m_gameObjects.push_back(new GameObject(m_current_uuid++));
    m_quadtree->insert(*m_gameObjects.back());
}

void SceneModule::DetachGameObject(GameObject* gameObject)
{
    m_gameObjects.erase(
        std::remove(m_gameObjects.begin(), m_gameObjects.end(), gameObject),
        m_gameObjects.end()
    );
}

void SceneModule::DestroyGameObject(GameObject* gameObject)
{
    DetachGameObject(gameObject);
    delete gameObject;
}

void SceneModule::AddGameObject(GameObject* gameObject) {
	m_gameObjects.push_back(gameObject);
}

void SceneModule::getGameObjectToRender(std::vector<GameObject*>& renderableGameObjects)
{

}


