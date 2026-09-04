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

	ComponentRef<Transform> m_door1;
	ComponentRef<Transform> m_bridge1;
	ComponentRef<Transform> m_bridge2;
	ComponentRef<Transform> m_door2;
	ComponentRef<Transform> m_door3;
	ComponentRef<Transform> m_navBlocker1;
	ComponentRef<Transform> m_navBlocker2;
	ComponentRef<Transform> m_navBlocker3;
	ComponentRef<Transform> m_navBlocker4;

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

    FieldList getExposedFields() const override;
};

