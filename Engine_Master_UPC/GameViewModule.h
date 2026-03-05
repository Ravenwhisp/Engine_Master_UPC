#pragma once
#include "Module.h"

class SceneModule;
class GameObject;

class GameViewModule : public Module
{
private:
	SceneModule*								m_sceneModule;
	std::vector<std::unique_ptr<GameObject>>	m_gameObjects;

public:
	GameViewModule();
	~GameViewModule();

	bool init() override;
	void update() override;
	//void render() override;
	void startGameSimulation();
	void stopGameSimulation();
};
