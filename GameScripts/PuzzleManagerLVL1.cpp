#include "pch.h"
#include "PuzzleManagerLVL1.h"

IMPLEMENT_SCRIPT_FIELDS(PuzzleManagerLVL1,
	SERIALIZED_COMPONENT_REF(m_door1, "Door1", ComponentType::TRANSFORM)
)

PuzzleManagerLVL1::PuzzleManagerLVL1(GameObject* owner)
    : Script(owner)
{
}

void PuzzleManagerLVL1::Start()
{
	m_puzzles[0] = { 0, 2, false };
	m_puzzles[1] = { 0, 2, false };
	m_puzzles[2] = { 0, 1, false };
}

void PuzzleManagerLVL1::Update()
{

}

void PuzzleManagerLVL1::puzzle1Solved()
{
	Debug::log("Puzzle 1 solved! Opening door...");
	TransformAPI::setRotationEuler(m_door1.getReferencedComponent(), Vector3(0.0f, 90.0f, 0.0f));
}

void PuzzleManagerLVL1::onCrystalsActivated(int puzzleID)
{
	if(m_puzzles.find(puzzleID) == m_puzzles.end())
	{
		Debug::log("Invalid puzzle ID: %d", puzzleID);
		return;
	}

	PuzzleData& puzzle = m_puzzles[puzzleID];
	
	if(puzzle.puzzleSolved)
	{
		Debug::log("Puzzle %d already solved, ignoring crystal activation.", puzzleID);
		return;
	}

	puzzle.crystalsActivated++;

	Debug::log("Crystal activated! Total activated: %d/%d", puzzle.crystalsActivated, puzzle.totalCrystals);
	if (isPuzzleSolved(puzzleID))
	{
		onPuzzleSolved(puzzleID);
	}
}

bool PuzzleManagerLVL1::isPuzzleSolved(int puzzleID) const
{
	const PuzzleData& puzzle = m_puzzles.at(puzzleID);
	return puzzle.crystalsActivated >= puzzle.totalCrystals;
}

void PuzzleManagerLVL1::onPuzzleSolved(int puzzleID)
{
	m_puzzles[puzzleID].puzzleSolved = true;
	Debug::log("Puzzle %d solved!", puzzleID);

	switch (puzzleID)
	{
	case 0:
		puzzle1Solved();
		break;
	default:
		Debug::log("No solution implemented for puzzle ID: %d", puzzleID);
		break;
	}
}

void PuzzleManagerLVL1::onCrystalsDeactivated(int puzzleID)
{
	if(m_puzzles.find(puzzleID) == m_puzzles.end())
	{
		Debug::log("Invalid puzzle ID: %d", puzzleID);
		return;
	}

	PuzzleData& puzzle = m_puzzles[puzzleID];
	if(puzzle.puzzleSolved)
	{
		Debug::log("Puzzle %d already solved, ignoring crystal deactivation.", puzzleID);
		return;
	}
	puzzle.crystalsActivated--;
	Debug::log("Crystal deactivated! Total activated: %d/%d", puzzle.crystalsActivated, puzzle.totalCrystals);
}

IMPLEMENT_SCRIPT(PuzzleManagerLVL1)