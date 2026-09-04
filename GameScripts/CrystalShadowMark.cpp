#include "pch.h"
#include "CrystalShadowMark.h"
#include "EnvironmentSound.h"
#include "EnemyDamageable.h"
#include "CrystalVisuals.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(CrystalShadowMark, EnemyShadowMark,
    SERIALIZED_COMPONENT_REF(m_puzzleManager, "PuzzleManager", ComponentType::TRANSFORM),
	SERIALIZED_INT(m_puzzleID, "Puzzle ID"),
	SERIALIZED_FLOAT(m_activeTime, "Active Time", 0.0f, 10.0f, 0.1f),
    SERIALIZED_ASSET_REF(m_crystalSparks, "Crystal Sparks Particle", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_crystalStars, "Crystal Stars Particle", AssetType::PREFAB)
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
    managerScript2 = GameObjectAPI::findScript<PuzzleManagerLVL2>(managerObject);
    if (managerScript == nullptr && managerScript2 == nullptr)
    {
        Debug::log("[CrystalMark] ERROR: PuzzleManager script not found on referenced object!");
    }

    m_visualsController = GameObjectAPI::findScript<CrystalVisuals>(getOwner());

    if (m_visualsController == nullptr)
    {
        Debug::warn("[CrystalMark] CrystalVisuals not found on '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void CrystalShadowMark::Update() 
{
    EnemyShadowMark::Update();

    if (!m_puzzleCompleted && managerScript != nullptr && managerScript->isPuzzleSolved(m_puzzleID))
    {
        completeCrystal();
    }
    if (!m_puzzleCompleted && managerScript2 != nullptr && managerScript2->isPuzzleSolved(m_puzzleID))
    {
        completeCrystal();
    }

    if (m_puzzleCompleted)
    {
        return;
    }

    if (!m_activated) 
    {
        return;
    }

	m_activationTimer += Time::getDeltaTime();
    
    if (m_activationTimer < m_activeTime)
    {
        return;
    }
   
    Debug::log("[CrystalMark] Crystal deactivated after %.1f seconds.", m_activeTime);

    m_activated = false;
    m_activationTimer = 0.0f;

    if (m_visualsController != nullptr)
    {
        m_visualsController->setActivated(false);
    }

    deactivateEffect();

    if (managerScript != nullptr)
    {
        managerScript->onCrystalsDeactivated(m_puzzleID);
    }
    else if (managerScript2 != nullptr)
    {
        managerScript2->onCrystalsDeactivated(m_puzzleID);
    }
    else
    {
        Debug::log("[CrystalMark] WARNING: PuzzleManager not found!");
    }
}

bool CrystalShadowMark::processAttack(PlayerAttackType attackType)
{
    if (m_activated || m_puzzleCompleted)
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
    const Vector3 effectPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())) + Vector3(0.0f, 1.0f, 0.0f);

    if (effectObject == nullptr)
    {
        effectObject = GameObjectAPI::instantiatePrefab(m_crystalSparks.m_id, effectPosition, Vector3(0.0f, 0.0f, 0.0f));
    }

    if (effectObject2 == nullptr)
    {
        effectObject2 = GameObjectAPI::instantiatePrefab(m_crystalStars.m_id, effectPosition, Vector3(0.0f, 0.0f, 0.0f));
    }
}

void CrystalShadowMark::deactivateEffect()
{
    if (effectObject != nullptr)
    {
        GameObjectAPI::removeGameObject(effectObject);
        effectObject = nullptr;
    }

    if (effectObject2 != nullptr)
    {
        GameObjectAPI::removeGameObject(effectObject2);
        effectObject2 = nullptr;
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

    if (managerScript == nullptr && managerScript2 == nullptr)
    {
        Debug::log("[CrystalMark] WARNING: PuzzleManagerLVL2 not found!");
        return;
    }
    

    m_activationTimer = 0.0f;
    m_activated = true;

    if (m_visualsController != nullptr)
    {
        m_visualsController->setActivated(true);
    }

    activeEffect();
    if (managerScript != nullptr)
    {
        Debug::log("[CrystalMark] '%s' activating puzzle %d using manager '%s'.", GameObjectAPI::getName(getOwner()), m_puzzleID, GameObjectAPI::getName(managerObject));
        managerScript->onCrystalsActivated(m_puzzleID);

        if (managerScript->isPuzzleSolved(m_puzzleID))
        {
            completeCrystal();
        }
    }

    if (managerScript2 != nullptr)
    {
        Debug::log("[CrystalMark] '%s' activating puzzle %d using manager '%s'.", GameObjectAPI::getName(getOwner()), m_puzzleID, GameObjectAPI::getName(managerObject));
        managerScript2->onCrystalsActivated(m_puzzleID);

        if (managerScript2->isPuzzleSolved(m_puzzleID))
        {
            completeCrystal();
        }
    }
    
}

void CrystalShadowMark::completeCrystal()
{
    if (m_puzzleCompleted)
    {
        return;
    }

    m_puzzleCompleted = true;
    m_activated = true;
    m_activationTimer = 0.0f;

    if (m_visualsController != nullptr)
    {
        m_visualsController->setActivated(true);
    }

    activeEffect();

    EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(getOwner());

    if (damageable)
    {
        damageable->setInvulnerable(true);
    }

    Debug::log("[CrystalMark] Crystal permanently activated for solved puzzle %d.", m_puzzleID);
}

IMPLEMENT_SCRIPT(CrystalShadowMark)
