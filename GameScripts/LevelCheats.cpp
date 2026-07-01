#include "pch.h"
#include "LevelCheats.h"
#include "KeyCode.h"
#include "PlayerState.h"
#include "PlayerDamageable.h"
#include "PlayerController.h"
#include "EnemyDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(LevelCheats,
    FIELD_GROUP_LABEL("Cheats")
)

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
    if (KeyComboPressed(KeyCode::T)) ToggleInvincibility();
    if (KeyComboPressed(KeyCode::Num3)) SpawnEnemy(0);
    if (KeyComboPressed(KeyCode::Num4)) SpawnEnemy(1);
	if (KeyComboPressed(KeyCode::Up)) SpawnEnemy(2);
	if (KeyComboPressed(KeyCode::Down)) SpawnEnemy(3);
    if (KeyComboPressed(KeyCode::D)) restartLevel();
	if (KeyComboPressed(KeyCode::F)) killEnemies();
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
	SceneAPI::requestSceneChange("Win_Scene");
}

void LevelCheats::AutoLose()
{
    Debug::log("AutoLose activated!");
    SceneAPI::requestSceneChange("Lose_Scene");
}

void LevelCheats::Teleport()
{
    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    for (GameObject* player : players)
    {
        Transform* playerTransform = GameObjectAPI::getTransform(player);
        TransformAPI::setGlobalPosition(playerTransform, spawnPoints[m_spawnIndex]);  
    }
    m_spawnIndex += 1;
    if (m_spawnIndex >= spawnPoints.size())
    {
        m_spawnIndex = 0;
    }
}

void LevelCheats::ToggleInvincibility()
{
    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    for (GameObject* player : players)
    {
        Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);

        if (damageable)
        {
            bool newInvulnerableState = !damageable->isInvulnerable();
            damageable->setInvulnerable(newInvulnerableState);
        }
    }
}

void LevelCheats::SpawnEnemy(int enemyPrefabIndex)
{
    if (enemyPrefabIndex < 0 || enemyPrefabIndex >= static_cast<int>(m_enemyPrefabPaths.size()))
    {
        Debug::warn("[LevelCheats] Invalid enemy prefab index: %i", enemyPrefabIndex);
        return;
    }

    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    if (players.empty())
    {
        Debug::warn("[LevelCheats] Cannot spawn enemy. No players found.");
        return;
    }

    if (m_playerIndex < 0 || m_playerIndex >= static_cast<int>(players.size()))
    {
        Debug::warn("[LevelCheats] Invalid player index: %i", m_playerIndex);
        return;
    }

    GameObject* player = players[m_playerIndex];
    Transform* playerTransform = GameObjectAPI::getTransform(player);

    if (playerTransform == nullptr)
    {
        Debug::warn("[LevelCheats] Cannot spawn enemy. Player has no Transform.");
        return;
    }

    Vector3 playerPosition = TransformAPI::getGlobalPosition(playerTransform);
    Vector3 enemySpawnPosition = playerPosition + Vector3(2.0f, 0.0f, 0.0f);

    const AssetRef<Prefab>& prefabRef = m_enemyPrefabPaths[enemyPrefabIndex];
    Debug::log("[LevelCheats] Spawning enemy prefab: UID %llu", prefabRef.m_ref.m_uid);
    GameObjectAPI::instantiatePrefab(prefabRef.m_ref, enemySpawnPosition, Vector3(0.0f, 0.0f, 0.0f));
}

void LevelCheats::RestoreHealth()
{
    Debug::log("Restore Health activated! Player %i healed", m_playerIndex + 1);

    GameObject* player = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER)[m_playerIndex];
    Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);

    if (damageable)
    {
        damageable->heal(100);
    }
}

void LevelCheats::DownState()
{
    Debug::log("Down State activated! Player %i downed", m_playerIndex + 1);

    GameObject* player = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER)[m_playerIndex];
    Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);

    if (damageable)
    {
        damageable->takeDamage(damageable->getCurrentHp());
    }
}

void LevelCheats::restartLevel()
{
    Debug::log("Restart Level activated!");
    SceneAPI::requestSceneChange("Level1");
}

void LevelCheats::killEnemies()
{
    Debug::log("Kill Enemies activated!");

    std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
    for (GameObject* enemy : enemies)
    {
        Damageable* damageable = GameObjectAPI::findScript<Damageable>(enemy);

        if (damageable)
        {
            damageable->takeDamage(damageable->getCurrentHp());
        }
    }
}

IMPLEMENT_SCRIPT(LevelCheats)
