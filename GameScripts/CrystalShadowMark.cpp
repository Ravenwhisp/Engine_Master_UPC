#include "pch.h"
#include "CrystalShadowMark.h"
#include "EnvironmentSound.h"
#include "EnemyDamageable.h"
#include "CrystalVisuals.h"
#include "ParticleLifecycle.h"
#include "ObjectVfxIds.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(CrystalShadowMark, EnemyShadowMark,
    SERIALIZED_COMPONENT_REF(m_puzzleManager, "PuzzleManager", ComponentType::TRANSFORM),
	SERIALIZED_INT(m_puzzleID, "Puzzle ID"),
	SERIALIZED_FLOAT(m_activeTime, "Active Time", 0.0f, 10.0f, 0.1f)
)

CrystalShadowMark::CrystalShadowMark(GameObject* owner) : EnemyShadowMark(owner) {}

void CrystalShadowMark::Start()
{
    EnemyShadowMark::Start();

	m_owner = getOwner();

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

void CrystalShadowMark::OnGameStop()
{
    ParticleLifecycle::destroy(m_effectObject);
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

void CrystalShadowMark::ensureEffect()
{
    const Vector3 effectPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())) + Vector3(0.0f, 1.0f, 0.0f);
    ParticleLifecycle::ensurePersistent(m_effectObject, ObjectVfxIds::crystalActiveEffect(), effectPosition, Vector3::Zero, m_owner );
}

void CrystalShadowMark::activeEffect()
{
    ensureEffect();

    if (m_effectObject == nullptr)
    {
        Debug::warn("[CrystalMark] Could not instantiate crystal effect on '%s'.", GameObjectAPI::getName(getOwner()));
        return;
    }

    Transform* effectTransform = GameObjectAPI::getTransform(m_effectObject);
    if (effectTransform != nullptr)
    {
        const Vector3 effectPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())) + Vector3(0.0f, 1.0f, 0.0f);
        TransformAPI::setGlobalPosition(effectTransform, effectPosition);
    }

    ParticleLifecycle::activate(m_effectObject);
}

void CrystalShadowMark::deactivateEffect()
{
    ParticleLifecycle::deactivate(m_effectObject);
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
