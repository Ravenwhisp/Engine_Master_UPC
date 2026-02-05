#include "Globals.h"
#include "SceneModule.h"

bool SceneModule::init()
{
    return true;
}

void SceneModule::update()
{

}
void SceneModule::preRender()
{

}

void SceneModule::CreateGameObject()
{
    m_gameObjects.push_back(new GameObject(m_current_uuid++));
}

void SceneModule::RemoveGameObject(const short id)
{
    
}

void SceneModule::getGameObjectToRender(std::vector<GameObject*>& renderableGameObjects)
{

}

void SceneModule::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) {

}
