#include "Globals.h"
#include "SceneModule.h"

using namespace DirectX::SimpleMath;

bool SceneModule::init()
{
    auto rectangle = RectangleData(0, 0, 10, 10);
    m_quadtree = new Quadtree(rectangle);

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
    m_quadtree->insert(*m_gameObjects.back());
}

void SceneModule::RemoveGameObject(const short id)
{
    
}

void SceneModule::getGameObjectToRender(std::vector<GameObject*>& renderableGameObjects)
{

}

void SceneModule::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) {

}
