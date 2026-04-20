#pragma once

#include "ScriptAPI.h"
#include <unordered_map>

class LevelCheats : public Script
{
    DECLARE_SCRIPT(LevelCheats)

public:
    explicit LevelCheats(GameObject* owner);

    void Start() override;
    void Update() override;

    void AutoWin();
	void AutoLose();
    void Teleport();
	void ActivateGodMode();
	void SpawnEnemies();
	void RestoreHealth();
	void DownState();
	void restartLevel();

    //ScriptFieldList getExposedFields() const override;

private:

	int m_playerIndex = 0;
    int m_spawnIndex = 0;

    std::vector<Vector3> spawnPoints = {
    Vector3(11.46f, 1.156f, 31.93f),
    Vector3(20.679f, 1.156f, -16.721f),
    Vector3(-8.706f, 0.0f, 29.411f),
    Vector3(-44.055f, 4.275f, -14.0f),
    Vector3(-64.385f, -22.566f, 22.565f),
    Vector3(-45.819f, -16.11f, 81.79f),
    Vector3(-45.233f, -16.11f, 123.369f)
    };

	bool KeyComboPressed(KeyCode mainKey);
    std::unordered_map<KeyCode, bool> m_previousKeyStates;
};

