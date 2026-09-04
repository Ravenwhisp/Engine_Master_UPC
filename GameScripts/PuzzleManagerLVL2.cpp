#include "pch.h"
#include "PuzzleManagerLVL2.h"
#include "EnvironmentSound.h"

namespace
{
	constexpr const char* k_openDoor = "Play_Environment_Open_Door";

	// Plays the door-open SFX from the door's own GameObject (positional 3D).
	void playDoorOpen(const ComponentRef<Transform>& doorRef)
	{
		Transform* doorTransform = doorRef.getReferencedComponent();
		if (doorTransform != nullptr)
		{
			EnvironmentSound::play(ComponentAPI::getOwner(doorTransform), k_openDoor);
		}
	}
}

IMPLEMENT_SCRIPT_FIELDS(PuzzleManagerLVL2,
	SERIALIZED_COMPONENT_REF(m_door1, "Door1", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_navBlocker1, "NavBlocker1", ComponentType::TRANSFORM),
)

PuzzleManagerLVL2::PuzzleManagerLVL2(GameObject* owner)
	: Script(owner)
{
}

void PuzzleManagerLVL2::Start()
{
	m_puzzles[0] = { 0, 3, false };

	blocker1 = m_navBlocker1.getReferencedComponent()->getOwner();

}

void PuzzleManagerLVL2::Update()
{
}

void PuzzleManagerLVL2::puzzle1Solved()
{
	Debug::log("Puzzle 1 solved! Opening door...");
	TransformAPI::setRotationEuler(m_door1.getReferencedComponent(), Vector3(0.0f, -90.0f, 0.0f));
	playDoorOpen(m_door1);

	NavRuntimeBlockerComponent* blocker1Comp = NavigationAPI::getRuntimeBlockerComponent(blocker1);
	NavigationAPI::setBlocked(blocker1Comp, false);

}

void PuzzleManagerLVL2::onCrystalsActivated(int puzzleID)
{
	if (m_puzzles.find(puzzleID) == m_puzzles.end())
	{
		Debug::log("Invalid puzzle ID: %d", puzzleID);
		return;
	}

	PuzzleData& puzzle = m_puzzles[puzzleID];

	if (puzzle.puzzleSolved)
	{
		Debug::log("Puzzle %d already solved, ignoring crystal activation.", puzzleID);
		return;
	}

	puzzle.crystalsActivated++;

	Debug::log("Crystal activated! Total activated: %d/%d", puzzle.crystalsActivated, puzzle.totalCrystals);
	if (puzzle.crystalsActivated >= puzzle.totalCrystals)
	{
		onPuzzleSolved(puzzleID);
	}
}

bool PuzzleManagerLVL2::isPuzzleSolved(int puzzleID) const
{
	const auto it = m_puzzles.find(puzzleID);

	if (it == m_puzzles.end())
	{
		return false;
	}

	return it->second.puzzleSolved;
}

void PuzzleManagerLVL2::onPuzzleSolved(int puzzleID)
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

void PuzzleManagerLVL2::onCrystalsDeactivated(int puzzleID)
{
	if (m_puzzles.find(puzzleID) == m_puzzles.end())
	{
		Debug::log("Invalid puzzle ID: %d", puzzleID);
		return;
	}

	PuzzleData& puzzle = m_puzzles[puzzleID];
	if (puzzle.puzzleSolved)
	{
		Debug::log("Puzzle %d already solved, ignoring crystal deactivation.", puzzleID);
		return;
	}
	puzzle.crystalsActivated--;
	Debug::log("Crystal deactivated! Total activated: %d/%d", puzzle.crystalsActivated, puzzle.totalCrystals);
}

IMPLEMENT_SCRIPT(PuzzleManagerLVL2)
