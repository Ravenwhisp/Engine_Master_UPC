#include "pch.h"
#include "EnemyAttackState.h"

#include "EnemyBaseController.h"
#include "EnemyBaseAttackConfig.h"
#include "EnemySound.h"

#include "Damageable.h"
#include "PlayerState.h"
#include "PaladinVFX.h"

EnemyAttackState::EnemyAttackState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyAttackState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_paladinVFX = GameObjectAPI::findScript<PaladinVFX>(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedDamage = false;
    m_committedTarget = nullptr;

    if (!m_controller)
    {
        Debug::error("[EnemyAttackState] EnemyController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[EnemyAttackState] AnimationComponent not found.");
        return;
    }

    m_controller->clearPath();
    m_controller->resetRepathTimer();

    m_controller->updateCurrentTarget();
    m_committedTarget = m_controller->getCurrentTarget();

    m_enemySound = GameObjectAPI::findScript<EnemySound>(getOwner());
    if (m_enemySound)
    {
        m_enemySound->playBasicTelegraph();   // Paladin swing / Archer bow release
    }

    Debug::log("[EnemyAttackState] ENTER");
}

void EnemyAttackState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (m_controller->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_controller->trySendStunTrigger(m_animation))
    {
        return;
    }

    m_controller->faceCurrentTarget();

    m_stateTimer += Time::getDeltaTime();

    const EnemyBaseAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    if (!m_hasAppliedDamage && m_stateTimer >= cfg->m_basicAttackWindupTime)
    {
        playBasicAttackEffect();
        tryDamageTarget(m_committedTarget);

        if (m_enemySound)
        {
            m_enemySound->playBasicImpact();   // contact frame
        }

        m_hasAppliedDamage = true;
    }

    if (m_stateTimer >= cfg->m_basicAttackTotalDuration)
    {
        m_controller->updateCurrentTarget();

        if (!m_controller->hasValidTarget())
        {
            AnimationAPI::sendTrigger(m_animation, "ToIdle");
            Debug::log("[EnemyAttackState] Attack finished, Idle trigger sent");
        }
        else
        {
            AnimationAPI::sendTrigger(m_animation, "ToChase");
            Debug::log("[EnemyAttackState] Attack finished, Chase trigger sent");
        }

        return;
    }
}

void EnemyAttackState::OnStateExit()
{
    Debug::log("[EnemyAttackState] EXIT");
}

void EnemyAttackState::tryDamageTarget(Transform* targetTransform)
{
    const EnemyBaseAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    if (!targetTransform)
    {
        return;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (!targetObject)
    {
        return;
    }

    PlayerState* playerState = GameObjectAPI::findScript<PlayerState>(targetObject);
    if (playerState && playerState->isDowned())
    {
        return;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(targetObject);
    if (!damageable)
    {
        return;
    }

    damageable->takeDamage(cfg->m_basicAttackDamage);

    Debug::log("[EnemyAttackState] Damaged '%s' for %.2f.", GameObjectAPI::getName(targetObject), cfg->m_basicAttackDamage);
}

void EnemyAttackState::playBasicAttackEffect()
{
    if (m_paladinVFX)
    {
        m_paladinVFX->playBasicAttackEffect();
    }
}

IMPLEMENT_SCRIPT_FIELDS(EnemyAttackState,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)