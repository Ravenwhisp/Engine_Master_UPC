#pragma once
#include "Module.h"
#include "SceneSnapshot.h"

class ModuleScene;
class ModuleInput;
class SceneSnapshot;
class ModuleParticleSystem;

class GameObject;

class ModuleGameView : public Module
{
private:
	ModuleScene*								m_moduleScene;
	ModuleInput*								m_moduleInput;
	ModuleParticleSystem*						m_moduleParticleSystem;

	std::unique_ptr<SceneSnapshot>				m_sceneCloned;

	bool										m_showDebugWindow;
	bool m_pendingStop = false;

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
