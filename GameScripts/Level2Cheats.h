#pragma once

#include "ScriptAPI.h"
#include <unordered_map>

class Level2Cheats : public Script
{
    DECLARE_SCRIPT(Level2Cheats)

public:
    explicit Level2Cheats(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    void AutoWin();
    void AutoLose();
    void Teleport();
    void ToggleInvincibility();
    void SpawnEnemy(int enemyPrefabIndex);

private:

    int m_playerIndex = 0;
    int m_spawnIndex = 0;

    std::vector<Vector3> spawnPoints = {
    Vector3(20.494f, 1.274f, 10.265f),
    Vector3(20.64f, 1.274f, 32.483f),
    Vector3(38.555f, -4.882f, 10.795f),
    Vector3(47.647f, 1.274, 46.589f),
    Vector3(43.675f, -25.063f, 41.305f),
    Vector3(15.384f, -24.431f, 40.147f),
    Vector3(15.384f, -45.432f, 40.147f),
    Vector3(15.384f, -70.399f, 40.147f)
    };

    bool KeyComboPressed(KeyCode mainKey);
    std::unordered_map<KeyCode, bool> m_previousKeyStates;

    std::vector<AssetRef<Prefab>> m_enemyPrefabPaths;
};

