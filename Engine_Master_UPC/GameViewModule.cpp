#include "Globals.h"
#include "GameViewModule.h"

#include "Application.h"
#include "SceneModule.h"
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
	return true;
}

void GameViewModule::update()
{
	if(app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
	{
		// Here is where the game simulation logic would go, but for now we don't have anything to update in the GameViewModule
	}
}

void GameViewModule::startGameSimulation()
{
	// When we hit play, we create an exact copy of the game objects in the scene, so that we can restore them when we hit stop
	m_gameObjects = m_sceneModule->getAllGameObjects();
}

void GameViewModule::stopGameSimulation()
{
	// When we hit stop, we restore the scene's game objects with the copy we created when we hit play
	m_sceneModule->resetGameObjects(m_gameObjects);
}
