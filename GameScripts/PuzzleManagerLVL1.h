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

	void puzzle1Solved();

	void onCrystalsActivated(int puzzleID);
	void onCrystalsDeactivated(int puzzleID);

private:

	bool isPuzzleSolved(int puzzleId) const;
	void onPuzzleSolved(int puzzleId);

	std::unordered_map<int, PuzzleData> m_puzzles;

    ScriptFieldList getExposedFields() const override;
};

