#include "pch.h"
#include "Level2Cheats.h"
#include "KeyCode.h"
#include "PlayerState.h"
#include "PlayerDamageable.h"
#include "PlayerController.h"
#include "EnemyDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(Level2Cheats,
    FIELD_GROUP_LABEL("Cheats")
)

Level2Cheats::Level2Cheats(GameObject* owner)
    : Script(owner)
{
}

void Level2Cheats::Start()
{
}

void Level2Cheats::Update()
{
    if (KeyComboPressed(KeyCode::Q)) AutoWin();
    if (KeyComboPressed(KeyCode::W)) AutoLose();
    if (KeyComboPressed(KeyCode::E)) Teleport();
    if (KeyComboPressed(KeyCode::T)) ToggleInvincibility();
    if (KeyComboPressed(KeyCode::Num3)) SpawnEnemy(0);
    if (KeyComboPressed(KeyCode::Num4)) SpawnEnemy(1);
}

bool Level2Cheats::KeyComboPressed(KeyCode mainKey)
{
    bool isDown = Input::isKeyDown(mainKey) && Input::isKeyDown(KeyCode::RightShift);
    bool wasDown = m_previousKeyStates[mainKey];
    m_previousKeyStates[mainKey] = isDown;
    return isDown && !wasDown;
}

void Level2Cheats::AutoWin()
{
    Debug::warn("AutoWin activated!");
    SceneAPI::requestSceneChange("Win_Scene");
}

void Level2Cheats::AutoLose()
{
    Debug::log("AutoLose activated!");
    SceneAPI::requestSceneChange("Lose_Scene");
}

void Level2Cheats::Teleport()
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

void Level2Cheats::ToggleInvincibility()
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

void Level2Cheats::SpawnEnemy(int enemyPrefabIndex)
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

IMPLEMENT_SCRIPT(Level2Cheats)
