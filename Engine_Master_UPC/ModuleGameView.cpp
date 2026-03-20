#include "Globals.h"
#include "ModuleGameView.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleInput.h"

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
	m_sceneCloned = std::unique_ptr<SceneSnapshot>(m_moduleScene->takeSnapshot());
	instantiateScriptsOnPlay();
}

void ModuleGameView::stopGameSimulation()
{
	m_moduleScene->loadFromSnapshot(*m_sceneCloned.get());
	m_sceneCloned.reset();
}

void ModuleGameView::instantiateScriptsOnPlay() {
	// scripts instantiation
	for (GameObject* gameObject : m_moduleScene->getScene()->getAllGameObjects())
	{
		ScriptComponent* scriptComponent = gameObject->GetComponentAs<ScriptComponent>(ComponentType::SCRIPT);
		if (!scriptComponent || scriptComponent->getScriptName().empty())
		{
			continue;
		}

		if (!scriptComponent->getScript())
		{
			bool created = scriptComponent->createScriptInstance();
			assert(created);
		}

		scriptComponent->resetStartState();
	}
}
