#pragma once
#include "Module.h"

#include "ModuleScene.h"
#include "SceneSnapshot.h"

class ModuleScene;
class ModuleInput;

class GameObject;

class ModuleGameView : public Module
{
private:
	ModuleScene*								m_moduleScene;
	ModuleInput*								m_moduleInput;

	//std::vector<std::unique_ptr<GameObject>>	m_gameObjects;
	SceneSnapshot								m_sceneCloned;

	bool										m_showDebugWindow;

public:
	ModuleGameView();
	~ModuleGameView();

	bool init() override;
	void update() override;
	
	void startGameSimulation();
	void stopGameSimulation();

	void instantiateScriptsOnPlay();

	bool getShowDebugWindow() { return m_showDebugWindow; }
};
