#pragma once
#include "ScriptAPI.h"
#include "EnemyShadowMark.h"
#include "PuzzleManagerLVL1.h"

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

private:
	void activeEffect();
	void deactivateEffect();

    void activateCrystal();

public:
    ComponentRef<Transform> m_puzzleManager;
    int m_puzzleID = 0;
    float m_activeTime = 5.0f;
    PrefabRef m_crystalSparks;

private:
    GameObject* managerObject = nullptr;
    PuzzleManagerLVL1* managerScript = nullptr;

    bool m_activated = false;
    bool m_activatedLoopStarted = false;   // crystal hum loop: start once, 3D attenuation handles audibility

    float m_activationTimer = 0.0f;

    GameObject* effectObject = nullptr;
};

