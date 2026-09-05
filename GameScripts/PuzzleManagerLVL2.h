#pragma once

#include "ScriptAPI.h"

class PuzzleManagerLVL2 : public Script
{
	DECLARE_SCRIPT(MyScript)

	struct PuzzleData
	{
		int crystalsActivated = 0;
		int totalCrystals = 0;
		bool puzzleSolved = false;
	};

public:
	explicit PuzzleManagerLVL2(GameObject* owner);

	void Start() override;
	void Update() override;

	ComponentRef<Transform> m_door1;
	ComponentRef<Transform> m_navBlocker1;

	GameObject* blocker1;

	void puzzle1Solved();

	void onCrystalsActivated(int puzzleID);
	void onCrystalsDeactivated(int puzzleID);

	bool isPuzzleSolved(int puzzleId) const;

private:

	void onPuzzleSolved(int puzzleId);

	std::unordered_map<int, PuzzleData> m_puzzles;

	FieldList getExposedFields() const override;
};

