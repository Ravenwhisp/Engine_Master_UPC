#pragma once
#include "Module.h"
#include "GameObject.h"
#include "Scene.h"

class GameCoreModule : public Module
{
public:
	void CreateGameObject(GameObject* gameObject);
	void RemoveGameObject(const ID_TYPE id);
private:
	std::vector<GameObject*> objects;
	std::vector<Emeika::Scene*> scenes;
};

