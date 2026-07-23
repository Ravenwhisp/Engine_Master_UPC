#include "pch.h"
#include "CrystalShadowMark.h"
#include "EnvironmentSound.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(CrystalShadowMark, EnemyShadowMark,
    SERIALIZED_COMPONENT_REF(m_puzzleManager, "PuzzleManager", ComponentType::TRANSFORM),
	SERIALIZED_INT(m_puzzleID, "Puzzle ID"),
	SERIALIZED_FLOAT(m_activeTime, "Active Time", 0.0f, 10.0f, 0.1f),
    SERIALIZED_ASSET_REF(m_crystalSparks, "Crystal Sparks Particle", AssetType::PREFAB)
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

        deactivateEffect();

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

bool CrystalShadowMark::processAttack(PlayerAttackType attackType)
{
    if (m_activated)
    {
        return false;
    }

    const bool markExploited = EnemyShadowMark::processAttack(attackType);

    if (markExploited)
    {
        activateCrystal();
    }

    return markExploited;
}

void CrystalShadowMark::activeEffect()
{
    if (effectObject == nullptr)
    {
        effectObject = GameObjectAPI::instantiatePrefab(m_crystalSparks.m_id, TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())) + Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
    }
}

void CrystalShadowMark::deactivateEffect()
{
    if (effectObject != nullptr)
    {
        GameObjectAPI::removeGameObject(effectObject);
        effectObject = nullptr;
    }
}

void CrystalShadowMark::activateCrystal()
{
    EnvironmentSound::play(getOwner(), "Play_Environment_Crystal_Activate");

    if (!m_activatedLoopStarted)
    {
        EnvironmentSound::play(getOwner(), "Play_Environment_Crystal_Activated");
        m_activatedLoopStarted = true;
    }

    if (managerScript == nullptr)
    {
        Debug::log("[CrystalMark] WARNING: PuzzleManagerLVL1 not found!");
        return;
    }

    m_activationTimer = 0.0f;
    m_activated = true;

    activeEffect();
    managerScript->onCrystalsActivated(m_puzzleID);
}

IMPLEMENT_SCRIPT(CrystalShadowMark)
