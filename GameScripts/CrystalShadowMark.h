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

    FieldList getExposedFields() const override;

    bool processAttack(PlayerAttackType attackType) override;
    bool isActivated() const { return m_activated; }

    bool isPuzzleCompleted() const { return m_puzzleCompleted; }


private:
	void activeEffect();
	void deactivateEffect();

    void activateCrystal();
    void completeCrystal();

public:
    ComponentRef<Transform> m_puzzleManager;
    int m_puzzleID = 0;
    float m_activeTime = 5.0f;
    PrefabRef m_crystalSparks;
    PrefabRef m_crystalStars;

private:
    GameObject* managerObject = nullptr;
    PuzzleManagerLVL1* managerScript = nullptr;
    PuzzleManagerLVL2* managerScript2 = nullptr;
    CrystalVisuals* m_visualsController = nullptr;

    bool m_activated = false;
    bool m_puzzleCompleted = false;
    bool m_activatedLoopStarted = false;   // crystal hum loop: start once, 3D attenuation handles audibility

    float m_activationTimer = 0.0f;

    GameObject* effectObject = nullptr;
    GameObject* effectObject2 = nullptr;
};

