#pragma once

#include "ScriptAPI.h"

class CombatAreaEvent;
class CrystalShadowMark;

class ElevatorManager : public Script
{
    DECLARE_SCRIPT(ElevatorManager)

public:
    explicit ElevatorManager(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

private:
    void resolveCombatAreas();
    void resolveCrystals();

    void enableArea(int waveIndex);
    void disableArea(int waveIndex);
    void setActiveRecursive(GameObject* obj, bool active);

    void beginWave(int waveIndex);

    void startWallScroll();
    void updateWallScroll();

    void startPlatformMove(int targetIndex);
    void updatePlatformMove();

    int getTotalWaves() const;

    void killWaveEnemies(int waveIndex);

    enum class State { Idle, CycleActive, PlatformMoving, Done };

public:
    std::vector<ComponentRef<Transform>> m_crystals;
    std::vector<ComponentRef<Transform>> m_combatAreaRoots;

    ComponentRef<Transform> m_wallTop;
    ComponentRef<Transform> m_wallBottom;
    float m_wallPieceHeight = 10.0f;
    float m_wallThreshold = 20.0f;
    float m_wallScrollSpeed = 2.0f;

    ComponentRef<Transform> m_platform;
    std::vector<ComponentRef<Transform>> m_platformTargets;
    float m_platformMoveDuration = 2.0f;
    float m_platformLerpPower = 1.0f;

    int m_wavesPerCycle = 2;

private:
    std::vector<CrystalShadowMark*> m_crystalScripts;
    std::vector<CombatAreaEvent*> m_combatAreas;

    State m_state = State::Idle;
    int m_currentCycle = 0;
    int m_wavesCompleted = 0;
    int m_wavesDoneInCycle = 0;

    bool m_wallsActive = false;

    bool m_platformMoving = false;
    bool m_movingToCombat = false;
    bool m_waitingForReset = false;
    float m_platformTimer = 0.0f;
    float m_platformStartY = 0.0f;
    float m_platformTargetY = 0.0f;

    int m_cheatWaveIndex = 0;
    bool m_cheatWasPressed = false;
};
