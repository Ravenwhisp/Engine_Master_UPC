#include "Globals.h"
#include "GameViewModule.h"

#include "Application.h"
#include "GameObject.h"

GameViewModule::GameViewModule()
{
}

GameViewModule::~GameViewModule()
{
}

bool GameViewModule::init()
{
	m_sceneModule = app->getSceneModule();
	
#ifdef GAME_RELEASE
	m_sceneModule->loadScene("main");
	app->setEngineState(ENGINE_STATE::PLAYING);
#endif

	return true;
}

void GameViewModule::update()
{
	if(app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
	{
		for (GameObject* gameObject : m_sceneModule->getAllGameObjects())
		{
			if (gameObject->GetActive())
			{
				gameObject->update();
			}
		}
	}
}

void GameViewModule::startGameSimulation()
{
	// When we hit play, we create an exact copy of the game objects in the scene, so that we can restore them when we hit stop
	m_sceneCloned = m_sceneModule->getClonedGameObjects();
}

void GameViewModule::stopGameSimulation()
{
	// When we hit stop, we restore the scene's game objects with the copy we created when we hit play
	m_sceneModule->resetGameObjects(std::move(m_sceneCloned));

	m_sceneCloned = SceneSnapshot();
}
