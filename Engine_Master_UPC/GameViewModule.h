#pragma once
#include "Module.h"
#include "SceneModule.h"

class SceneModule;
class InputModule;

class GameObject;

class GameViewModule : public Module
{
private:
	SceneModule*								m_sceneModule;
	InputModule*								m_inputModule;

	//std::vector<std::unique_ptr<GameObject>>	m_gameObjects;
	SceneSnapshot								m_sceneCloned;

	bool										m_showDebugWindow;

public:
	GameViewModule();
	~GameViewModule();

	bool init() override;
	void update() override;
	
	void startGameSimulation();
	void stopGameSimulation();

	void instantiateScriptsOnPlay();

	bool getShowDebugWindow() { return m_showDebugWindow; }
};
