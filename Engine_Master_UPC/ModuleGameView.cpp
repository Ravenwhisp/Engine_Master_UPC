#include "Globals.h"
#include "ModuleGameView.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleParticleSystem.h"
#include "ModuleScripting.h"

#include "Scene.h"
#include "SceneSnapshot.h"
#include "GameObject.h"
#include "ScriptComponent.h"

ModuleGameView::ModuleGameView() = default;
ModuleGameView::~ModuleGameView() = default;

bool ModuleGameView::init()
{
	m_moduleScene = app->getModuleScene();
	m_moduleInput = app->getModuleInput();
	m_moduleParticleSystem = app->getModuleParticleSystem();

	m_showDebugWindow = false;

	return true;
}

void ModuleGameView::update()
{
	static Keyboard::KeyboardStateTracker keyTracker;

	Keyboard::State state = Keyboard::Get().GetState();
	keyTracker.Update(state);

	if(app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
	{
		if (keyTracker.pressed.F3)
		{
			m_showDebugWindow = !m_showDebugWindow;
		}
	} 
	else
	{
		m_showDebugWindow = false;
	}

}

void ModuleGameView::startGameSimulation()
{	
	m_sceneCloned = std::unique_ptr<SceneSnapshot>(m_moduleScene->takeSnapshot());
	m_moduleParticleSystem->resetAllParticles();

	app->getModuleScripting()->instantiateSceneScripts();

	m_moduleScene->initializeRuntimeSceneSystems();
}

void ModuleGameView::stopGameSimulation()
{
	m_moduleScene->clearRuntimeSceneSystems();

	m_moduleScene->loadFromSnapshot(*m_sceneCloned.get());
	m_sceneCloned.reset();
}
