#pragma once
#include "ScriptAPI.h"
#include "EnemyShadowMark.h"
#include "PuzzleManagerLVL1.h"
#include "PuzzleManagerLVL2.h"

class CrystalVisuals;

class CrystalShadowMark : public EnemyShadowMark
{
    DECLARE_SCRIPT(CrystalShadowMark)
public:
    explicit CrystalShadowMark(GameObject* owner);

    void Start()  override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    bool processAttack(PlayerAttackType attackType) override;
    bool isActivated() const { return m_activated; }

    bool isPuzzleCompleted() const { return m_puzzleCompleted; }

public:
    ComponentRef<Transform> m_puzzleManager;
    int m_puzzleID = 0;
    float m_activeTime = 5.0f;

private:
    void activeEffect();
    void deactivateEffect();
    void ensureEffect();

    void activateCrystal();
    void completeCrystal();

    GameObject* managerObject = nullptr;
    PuzzleManagerLVL1* managerScript = nullptr;
    PuzzleManagerLVL2* managerScript2 = nullptr;
    CrystalVisuals* m_visualsController = nullptr;

    bool m_activated = false;
    bool m_puzzleCompleted = false;
    bool m_activatedLoopStarted = false;

    float m_activationTimer = 0.0f;

    GameObject* m_effectObject = nullptr;
};
