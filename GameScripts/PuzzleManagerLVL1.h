#pragma once

#include "ScriptAPI.h"

class PuzzleManagerLVL1 : public Script
{
    DECLARE_SCRIPT(MyScript)

struct PuzzleData
{
	int crystalsActivated = 0;
	int totalCrystals = 0;
	bool puzzleSolved = false;
};

public:
    explicit PuzzleManagerLVL1(GameObject* owner);

    void Start() override;
    void Update() override;

	ScriptComponentRef<Transform> m_door1;
	ScriptComponentRef<Transform> m_bridge1;
	ScriptComponentRef<Transform> m_bridge2;
	ScriptComponentRef<Transform> m_door2;
	ScriptComponentRef<Transform> m_door3;
	ScriptComponentRef<Transform> m_navBlocker1;
	ScriptComponentRef<Transform> m_navBlocker2;
	ScriptComponentRef<Transform> m_navBlocker3;
	ScriptComponentRef<Transform> m_navBlocker4;

	GameObject* blocker1;
	GameObject* blocker2;
	GameObject* blocker3;
	GameObject* blocker4;

	float m_bridgeLowerDuration = 2.0f;   // seconds the bridge takes to lower (chains play during)

	void puzzle1Solved();
	void puzzle2Solved();
	void puzzle3Solved();

	void onCrystalsActivated(int puzzleID);
	void onCrystalsDeactivated(int puzzleID);

private:

	bool isPuzzleSolved(int puzzleId) const;
	void onPuzzleSolved(int puzzleId);

	std::unordered_map<int, PuzzleData> m_puzzles;

	// Bridge lowering animation (puzzle 2): tween down over m_bridgeLowerDuration with the
	// chain loop, then a thud + open the path at the bottom.
	bool m_bridgeLowering = false;
	float m_bridgeLowerTimer = 0.0f;
	Vector3 m_bridge1StartPos, m_bridge1StartRot, m_bridge1TargetPos, m_bridge1TargetRot;
	Vector3 m_bridge2StartPos, m_bridge2StartRot, m_bridge2TargetPos, m_bridge2TargetRot;
	GameObject* m_bridgeSoundEmitter = nullptr;
	void updateBridgeLowering(float dt);

    ScriptFieldList getExposedFields() const override;
};

