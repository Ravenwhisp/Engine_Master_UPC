#include "Globals.h"
#include "ModuleGameView.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleD3D12.h"

#include "GameObject.h"
#include "ScriptComponent.h"

ModuleGameView::ModuleGameView()
{
	m_moduleScene = nullptr;
	m_moduleInput = nullptr;
}

ModuleGameView::~ModuleGameView()
{

}

bool ModuleGameView::init()
{
	m_moduleScene = app->getModuleScene();
	m_moduleInput = app->getModuleInput();

	m_showDebugWindow = false;

#ifdef GAME_RELEASE
	m_moduleScene->loadScene("main");
	app->setEngineState(ENGINE_STATE::PLAYING);
#endif

	return true;
}

void ModuleGameView::update()
{

	if (m_pendingStop)
	{
		m_pendingStop = false;
		m_moduleScene->resetGameObjects(std::move(m_sceneCloned));
		m_sceneCloned = SceneSnapshot();
	}


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
	// When we hit play, we create an exact copy of the game objects in the scene, so that we can restore them when we hit stop
	m_sceneCloned = m_moduleScene->getClonedGameObjects();

	instantiateScriptsOnPlay();
}

void ModuleGameView::stopGameSimulation()
{
	m_pendingStop = true;
}

void ModuleGameView::instantiateScriptsOnPlay() {
	// scripts instantiation
	for (GameObject* gameObject : m_moduleScene->getAllGameObjects())
	{
		ScriptComponent* scriptComponent = gameObject->GetComponentAs<ScriptComponent>(ComponentType::SCRIPT);
		if (scriptComponent && !scriptComponent->getScriptName().empty())
		{
			scriptComponent->destroyScriptInstance();

			bool created = scriptComponent->createScriptInstance();
			assert(created);
		}
	}
}
