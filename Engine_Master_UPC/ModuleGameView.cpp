#include "Globals.h"
#include "ModuleGameView.h"

#include "Application.h"
#include "ModuleInput.h"

#include "Scene.h"
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
	m_sceneCloned = m_moduleScene->getScene()->getClonedGameObjects();

	instantiateScriptsOnPlay();
}

void ModuleGameView::stopGameSimulation()
{
	// When we hit stop, we restore the scene's game objects with the copy we created when we hit play
	m_moduleScene->getScene()->resetGameObjects(std::move(m_sceneCloned));

	m_sceneCloned = SceneSnapshot();
}

void ModuleGameView::instantiateScriptsOnPlay() {
	// scripts instantiation
	for (GameObject* gameObject : m_moduleScene->getScene()->getAllGameObjects())
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
