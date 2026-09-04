#pragma once

#include "ScriptAPI.h"
#include "PersistingPowerupState.h"
#include <map>
#include <array>

enum SceneId
{
    NONE_SCENE = 0,
    LEVEL1,
    LEVEL2,
    LEVEL3
};

enum CheckpointId
{
	NONE = 0, // Start of the game

    CHECKPOINT_1_LEVEL_1 = 101,
    CHECKPOINT_2_LEVEL_1,
    CHECKPOINT_3_LEVEL_1,

    LEVEL_2 = 200, // Start of Level 2

	CHECKPOINT_1_LEVEL_2 = 201,

    LEVEL_3 = 300, // Start of Level 3

	CHECKPOINT_1_LEVEL_3 = 301,
	//... Add more checkpoints as needed
};

enum PuzzleId
{
    PUZZLE1_LEVEL1 = 0,
    PUZZLE2_LEVEL1,
    PUZZLE3_LEVEL1,
    COUNT
    //... Add more puzzle ids as needed
};

class PersistingCheckpointState
{
public:
    static PersistingCheckpointState& Get();

    void SetCheckpoint(CheckpointId checkpointId);

    void Reset();

    bool IsStartOfLevel() const { return m_lastCheckpointId % 100 == 0; };

public:
    CheckpointId m_lastCheckpointId = CheckpointId::NONE;
    SceneId m_lastSceneId = SceneId::NONE_SCENE;

    // elementos globales
    float m_savedReaperGaugeAmount = 0.f;
    bool m_savedUnlockedPowerups[static_cast<int>(PowerupId::Count)];

    float m_savedLyrielHealth = 0.f;
    float m_savedDeathHealth = 0.f;

    std::array<bool, static_cast<size_t>(PuzzleId::COUNT)> m_solvedPuzzles{};
    std::array<bool, static_cast<size_t>(PuzzleId::COUNT)> m_solvedPuzzlesPersistent{};

    // elementos correspondientes a cada nivel
    Vector3 m_savedLyrielRespawn;
    Vector3 m_savedDeathRespawn;

    std::vector<UID> m_deadEnemies;
    std::vector<UID> m_brokenBreakables;
    std::vector<UID> m_triggeredEvents;

    std::vector<UID> m_deadEnemiesPersistent;
    std::vector<UID> m_brokenBreakablesPersistent;
    std::vector<UID> m_triggeredEventsPersistent;

};