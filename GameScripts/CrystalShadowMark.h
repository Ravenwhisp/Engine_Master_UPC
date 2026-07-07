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

    void exploit();

    bool isActivated() const { return m_activated; }

    void notifyDeathHit();

    void updateUI();

    ScriptComponentRef<Transform> m_puzzleManager;

	int m_puzzleID = 0;

	float m_activeTime = 5.0f;

    std::string m_crystalSparks = "Assets/Prefabs/Particles/CrystalSparks.prefab";

private:

    bool m_activated = false;
    bool m_activatedLoopStarted = false;   // crystal hum loop: start once, 3D attenuation handles audibility

	float m_activationTimer = 0.0f;

	GameObject* effectObject = nullptr;

	void activeEffect();
	void deactivateEffect();

	GameObject* managerObject = nullptr;
	PuzzleManagerLVL1* managerScript = nullptr;

ScriptFieldList getExposedFields() const override;
};

