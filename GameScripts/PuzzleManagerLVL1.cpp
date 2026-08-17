#include "pch.h"
#include "PuzzleManagerLVL1.h"
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

IMPLEMENT_SCRIPT_FIELDS(PuzzleManagerLVL1,
	SERIALIZED_COMPONENT_REF(m_door1, "Door1", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_bridge1, "Bridge1", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_bridge2, "Bridge2", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_door2, "Door2", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_door3, "Door3", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_navBlocker1, "NavBlocker1", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_navBlocker2, "NavBlocker2", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_navBlocker3, "NavBlocker3", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_navBlocker4, "NavBlocker4", ComponentType::TRANSFORM),
	SERIALIZED_FLOAT(m_bridgeLowerDuration, "Bridge Lower Duration", 0.1f, 20.0f, 0.1f)
)

PuzzleManagerLVL1::PuzzleManagerLVL1(GameObject* owner)
    : Script(owner)
{
}

void PuzzleManagerLVL1::Start()
{
	m_puzzles[0] = { 0, 2, false };
	m_puzzles[1] = { 0, 2, false };
	m_puzzles[2] = { 0, 2, false };

	blocker1 = m_navBlocker1.getReferencedComponent()->getOwner();
	blocker2 = m_navBlocker2.getReferencedComponent()->getOwner();
	blocker3 = m_navBlocker3.getReferencedComponent()->getOwner();
	blocker4 = m_navBlocker4.getReferencedComponent()->getOwner();

}

void PuzzleManagerLVL1::Update()
{
	updateBridgeLowering(Time::getDeltaTime());
}

void PuzzleManagerLVL1::puzzle1Solved()
{
	Debug::log("Puzzle 1 solved! Opening door...");
	TransformAPI::setRotationEuler(m_door1.getReferencedComponent(), Vector3(0.0f, -90.0f, 0.0f));
	playDoorOpen(m_door1);

	NavRuntimeBlockerComponent* blocker1Comp = NavigationAPI::getRuntimeBlockerComponent(blocker1);
	NavRuntimeBlockerComponent* blocker2Comp = NavigationAPI::getRuntimeBlockerComponent(blocker2);
	NavigationAPI::setBlocked(blocker1Comp, false);
	NavigationAPI::setBlocked(blocker2Comp, true);
}

void PuzzleManagerLVL1::puzzle2Solved()
{
	Debug::log("Puzzle 2 solved! Lowering bridge...");

	Transform* bridge1 = m_bridge1.getReferencedComponent();
	Transform* bridge2 = m_bridge2.getReferencedComponent();
	if (!bridge1 || !bridge2)
	{
		return;
	}

	// Capture current (raised) transforms as the start of the lowering tween.
	m_bridge1StartPos = TransformAPI::getPosition(bridge1);
	m_bridge1StartRot = TransformAPI::getEulerDegrees(bridge1);
	m_bridge2StartPos = TransformAPI::getPosition(bridge2);
	m_bridge2StartRot = TransformAPI::getEulerDegrees(bridge2);

	// Final (lowered) transforms — the same values the puzzle used to snap to.
	m_bridge1TargetPos = Vector3(12.161f, 0.136f, -0.912f);
	m_bridge1TargetRot = Vector3(0.0f, 0.0f, 0.0f);
	m_bridge2TargetPos = Vector3(8.0f, 0.076f, -0.894f);
	m_bridge2TargetRot = Vector3(0.0f, 180.0f, 0.0f);

	m_bridgeSoundEmitter = ComponentAPI::getOwner(bridge1);
	m_bridgeLowering = true;
	m_bridgeLowerTimer = 0.0f;

	// Chains loop while it lowers (stopped + thud at the bottom in updateBridgeLowering).
	EnvironmentSound::play(m_bridgeSoundEmitter, "Play_Environment_Chain_Bridge");
}

void PuzzleManagerLVL1::updateBridgeLowering(float dt)
{
	if (!m_bridgeLowering)
	{
		return;
	}

	Transform* bridge1 = m_bridge1.getReferencedComponent();
	Transform* bridge2 = m_bridge2.getReferencedComponent();
	if (!bridge1 || !bridge2)
	{
		m_bridgeLowering = false;
		return;
	}

	m_bridgeLowerTimer += dt;

	float t = (m_bridgeLowerDuration > 0.0f) ? (m_bridgeLowerTimer / m_bridgeLowerDuration) : 1.0f;
	if (t > 1.0f)
	{
		t = 1.0f;
	}

	TransformAPI::setPosition(bridge1, MathAPI::lerp(m_bridge1StartPos, m_bridge1TargetPos, t));
	TransformAPI::setRotationEuler(bridge1, MathAPI::lerp(m_bridge1StartRot, m_bridge1TargetRot, t));
	TransformAPI::setPosition(bridge2, MathAPI::lerp(m_bridge2StartPos, m_bridge2TargetPos, t));
	TransformAPI::setRotationEuler(bridge2, MathAPI::lerp(m_bridge2StartRot, m_bridge2TargetRot, t));

	if (t >= 1.0f)
	{
		m_bridgeLowering = false;

		// Stop the chains and play the landing thud.
		EnvironmentSound::play(m_bridgeSoundEmitter, "Stop_Environment_Chain_Bridge");
		EnvironmentSound::play(m_bridgeSoundEmitter, "Play_Environment_Bridge_Fall");

		// Bridge is down → open the path.
		NavRuntimeBlockerComponent* blocker3Comp = NavigationAPI::getRuntimeBlockerComponent(blocker3);
		NavigationAPI::setBlocked(blocker3Comp, false);
	}
}

void PuzzleManagerLVL1::puzzle3Solved()
{
	Debug::log("Puzzle 3 solved!");
	TransformAPI::setRotationEuler(m_door2.getReferencedComponent(), Vector3(0.0f, 90.0f, 0.0f));
	TransformAPI::setRotationEuler(m_door3.getReferencedComponent(), Vector3(0.0f, -90.0f, 0.0f));
	playDoorOpen(m_door2);
	playDoorOpen(m_door3);

	NavRuntimeBlockerComponent* blocker4Comp = NavigationAPI::getRuntimeBlockerComponent(blocker4);
	NavigationAPI::setBlocked(blocker4Comp, false);
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
	case 1:
		puzzle2Solved();
		break;
	case 2:
		puzzle3Solved();
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