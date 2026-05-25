#include "pch.h"
#include "CrystalShadowMark.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(CrystalShadowMark, EnemyShadowMark,
    SERIALIZED_COMPONENT_REF(m_puzzleManager, "PuzzleManager", ComponentType::TRANSFORM),
	SERIALIZED_INT(m_puzzleID, "Puzzle ID"),
	SERIALIZED_FLOAT(m_activeTime, "Active Time", 0.0f, 10.0f, 0.1f)
)

CrystalShadowMark::CrystalShadowMark(GameObject* owner) : EnemyShadowMark(owner) {}

void CrystalShadowMark::Start()
{
    EnemyShadowMark::Start();

    managerObject = ComponentAPI::getOwner(m_puzzleManager.getReferencedComponent());
    if(managerObject == nullptr)
    {
        Debug::log("[CrystalMark] ERROR: PuzzleManager reference is invalid!");
        return;
	}
	managerScript = GameObjectAPI::findScript<PuzzleManagerLVL1>(managerObject);
    if (managerScript == nullptr)
    {
        Debug::log("[CrystalMark] ERROR: PuzzleManager script not found on referenced object!");
    }
}

void CrystalShadowMark::Update() 
{
    EnemyShadowMark::Update();

    if (!m_activated) return;

	m_activationTimer += Time::getDeltaTime();
    if (m_activationTimer >= m_activeTime)
    {
        Debug::log("[CrystalMark] Crystal deactivated after %.1f seconds.", m_activeTime);
        m_activated = false;
        m_activationTimer = 0.0f;

        if (managerScript != nullptr)
        {
            managerScript->onCrystalsDeactivated(m_puzzleID);
        }
        else
        {
            Debug::log("[CrystalMark] WARNING: PuzzleManagerLVL1 not found!");
        }
    }
}

void CrystalShadowMark::exploit()
{
    EnemyShadowMark::exploit(); 

    if (managerScript != nullptr)
    {
        managerScript->onCrystalsActivated(m_puzzleID);
    }
    else
    {
		Debug::log("[CrystalMark] WARNING: PuzzleManagerLVL1 not found!");
    }
}

IMPLEMENT_SCRIPT(CrystalShadowMark)
