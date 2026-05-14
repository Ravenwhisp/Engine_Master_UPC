#include "pch.h"
#include "LevelCheats.h"
#include "KeyCode.h"
#include "PlayerState.h"
#include "PlayerDamageable.h"
#include "PlayerController.h"

//static const ScriptFieldInfo myScriptFields[] =
//{
//    { "Level Name", ScriptFieldType::String, offsetof(LevelCheats, levelName) },
//    { "Win Scene Name", ScriptFieldType::String, offsetof(LevelCheats, winSceneName) },
//    { "Lose Scene Name", ScriptFieldType::String, offsetof(LevelCheats, loseSceneName)}
//};
//
//IMPLEMENT_SCRIPT_FIELDS(LevelCheats, myScriptFields)

LevelCheats::LevelCheats(GameObject* owner)
    : Script(owner)
{
}

void LevelCheats::Start()
{
}

void LevelCheats::Update()
{
    if (KeyComboPressed(KeyCode::Q)) AutoWin();
    if (KeyComboPressed(KeyCode::W)) AutoLose();
    if (KeyComboPressed(KeyCode::E)) Teleport();
    if (KeyComboPressed(KeyCode::R)) ActivateGodMode();
    if (KeyComboPressed(KeyCode::T)) SpawnEnemies();
    //if (KeyComboPressed(KeyCode::D)) restartLevel();
    if (Input::isKeyDown(KeyCode::RightShift) && Input::isKeyDown(KeyCode::A))
    {
        if (KeyComboPressed(KeyCode::Num1)) 
        { 
            m_playerIndex = 0; 
            RestoreHealth(); 
        }
        else if (KeyComboPressed(KeyCode::Num2)) 
        { 
            m_playerIndex = 1; 
            RestoreHealth(); 
        }
    }
    if (Input::isKeyDown(KeyCode::RightShift) && Input::isKeyDown(KeyCode::S))
    {
        if (KeyComboPressed(KeyCode::Num1)) 
        { 
            m_playerIndex = 0; 
            DownState(); 
        }
        else if (KeyComboPressed(KeyCode::Num2)) 
        { 
            m_playerIndex = 1; 
            DownState(); 
        }
    }
}

bool LevelCheats::KeyComboPressed(KeyCode mainKey)
{
    bool isDown = Input::isKeyDown(mainKey) && Input::isKeyDown(KeyCode::RightShift);
    bool wasDown = m_previousKeyStates[mainKey];
    m_previousKeyStates[mainKey] = isDown;
    return isDown && !wasDown;
}

void LevelCheats::AutoWin()
{
    Debug::warn("AutoWin activated!");
	SceneAPI::requestSceneChange("WinScene");
}

void LevelCheats::AutoLose()
{
    Debug::log("AutoLose activated!");
    SceneAPI::requestSceneChange("LoseScene");
}

void LevelCheats::Teleport()
{
    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    for (GameObject* player : players)
    {
        Transform* playerTransform = GameObjectAPI::getTransform(player);
        TransformAPI::setPosition(playerTransform, spawnPoints[m_spawnIndex]);  
    }
    m_spawnIndex += 1;
    if (m_spawnIndex >= spawnPoints.size())
    {
        m_spawnIndex = 0;
    }
}

void LevelCheats::ActivateGodMode()
{
    Debug::log("God Mode");
    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    for (GameObject* player : players)
    {
        Script* playerControllerScript = GameObjectAPI::getScript(player, "PlayerController");
		PlayerController* playerController = dynamic_cast<PlayerController*>(playerControllerScript);
        if (playerController)
        {
            playerController->m_godMode = !playerController->m_godMode;
		}
    }
	
    
}

void LevelCheats::SpawnEnemies()
{
    Debug::log("Spawn Enemies activated!");

    GameObject* player = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER)[m_playerIndex];
	Transform* playerTransform = GameObjectAPI::getTransform(player);
	Vector3 playerPosition = TransformAPI::getPosition(playerTransform);

	Vector3 enemySpawnPosition = playerPosition + Vector3(2.0f, 0.0f, 0.0f);

	GameObject* enemyPrefab = GameObjectAPI::instantiatePrefab("Assets/Prefabs/Spider.prefab", enemySpawnPosition, Vector3(0, 0, 0));
    
}

void LevelCheats::RestoreHealth()
{
    Debug::log("Restore Health activated! Player %i healed",m_playerIndex+1);
	GameObject* player = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER)[m_playerIndex];
    Script* damageableScript = GameObjectAPI::getScript(player, "PlayerDamageable");
    PlayerDamageable* damageable = dynamic_cast<PlayerDamageable*>(damageableScript);
    if (damageable)
    {
        damageable->heal(100);
	}
    
}

void LevelCheats::DownState()
{
    Debug::log("Down State activated! Player % i downed", m_playerIndex+1);
	GameObject* player = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER)[m_playerIndex];
    Script* damageableScript = GameObjectAPI::getScript(player, "PlayerState");
    PlayerState* playerState = dynamic_cast<PlayerState*>(damageableScript);
    if (playerState)
    {
        playerState->setState(PlayerStateType::Downed);
	}
    
}

void LevelCheats::restartLevel()
{
    Debug::log("Restart Level activated!");
    SceneAPI::requestSceneChange("LVL1Cheats");
}

IMPLEMENT_SCRIPT(LevelCheats)
