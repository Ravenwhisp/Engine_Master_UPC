#include "Globals.h"
#include "GameCoreModule.h"

void GameCoreModule::CreateGameObject(GameObject* gameObject)
{
    int numGameObjects = objects.size();
    ID_TYPE id = ID_TYPE(numGameObjects);
    gameObject->_id = id;
    objects.emplace_back(gameObject);
}

void GameCoreModule::RemoveGameObject(const ID_TYPE id)
{
}
