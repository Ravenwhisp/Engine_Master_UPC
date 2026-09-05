#include "pch.h"
#include "EnemyAttackState.h"

#include "EnemyBaseController.h"
#include "EnemyBaseAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "EnemySound.h"

#include "Damageable.h"
#include "PlayerState.h"
#include "PaladinVFX.h"
#include "PaladinUI.h"

namespace
{
    constexpr float PaladinBasicAttackWidth = 2.5f;
    constexpr float PaladinBasicAttackForwardOffset = 0.5f;
}

EnemyAttackState::EnemyAttackState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyAttackState::OnStateEnter()
{
    m_controller =
        GameObjectAPI::findScript<EnemyBaseController>(getOwner());

    m_attackExecutor =
        GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());

    m_animation =
        AnimationAPI::getAnimationComponent(getOwner());

    m_paladinVFX =
        GameObjectAPI::findScript<PaladinVFX>(getOwner());

    m_paladinUI =
        GameObjectAPI::findScript<PaladinUI>(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedDamage = false;
    m_committedTarget = nullptr;
    m_usePaladinAreaAttack = false;
    m_lockedAttackOrigin = Vector3::Zero;
    m_lockedAttackDirection = Vector3::Zero;
    m_lockedTargetPosition = Vector3::Zero;

    if (m_paladinUI)
    {
        m_paladinUI->hideBasicAttackUI();
    }

    if (!m_controller)
    {
        Debug::error(
            "[EnemyAttackState] EnemyController not found."
        );

        return;
    }

    if (!m_animation)
    {
        Debug::error(
            "[EnemyAttackState] AnimationComponent not found."
        );

        return;
    }

    m_controller->clearPath();
    m_controller->resetRepathTimer();

    m_controller->updateCurrentTarget();
    m_committedTarget = m_controller->getCurrentTarget();

    if (m_paladinVFX &&
        m_attackExecutor &&
        m_committedTarget)
    {
        lockPaladinAttackArea();
    }

    m_enemySound =
        GameObjectAPI::findScript<EnemySound>(getOwner());

    if (m_enemySound)
    {
        m_enemySound->playBasicTelegraph();
    }

    Debug::log("[EnemyAttackState] ENTER");
}

void EnemyAttackState::OnStateUpdate()
{
    if (!m_controller ||
        !m_controller->getAttackConfig() ||
        !m_animation)
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

    const EnemyBaseAttackConfig* attackConfig =
        m_controller->getAttackConfig();

    if (m_usePaladinAreaAttack && !m_hasAppliedDamage)
    {
        m_controller->facePosition(m_lockedTargetPosition);

        drawPaladinAttackTelegraph();

        if (m_paladinUI)
        {
            const Vector3 telegraphCenter =
                m_lockedAttackOrigin +
                m_lockedAttackDirection *
                (attackConfig->m_basicAttackRange * 0.5f);

            m_paladinUI->updateBasicAttackUIPose(
                telegraphCenter,
                m_lockedAttackDirection
            );
        }
    }
    else if (!m_usePaladinAreaAttack)
    {
        m_controller->faceCurrentTarget();
    }

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasAppliedDamage &&
        m_stateTimer >= attackConfig->m_basicAttackWindupTime)
    {
        if (m_usePaladinAreaAttack && m_paladinUI)
        {
            m_paladinUI->showBasicAttackImpact();
        }

        playBasicAttackEffect();

        if (m_usePaladinAreaAttack)
        {
            applyPaladinAreaDamage();

            if (m_paladinVFX)
            {
                m_paladinVFX->playShieldAttackHits(
                    m_lockedAttackOrigin,
                    m_lockedAttackDirection,
                    attackConfig->m_basicAttackRange,
                    PaladinBasicAttackWidth
                );
            }
        }
        else
        {
            tryDamageTarget(m_committedTarget);
        }

        if (m_enemySound)
        {
            m_enemySound->playBasicImpact();
        }

        m_hasAppliedDamage = true;
    }

    if (m_stateTimer >= attackConfig->m_basicAttackTotalDuration)
    {
        m_controller->updateCurrentTarget();

        if (!m_controller->hasValidTarget())
        {
            AnimationAPI::sendTrigger(
                m_animation,
                "ToIdle"
            );

            Debug::log(
                "[EnemyAttackState] Attack finished, Idle trigger sent"
            );
        }
        else
        {
            AnimationAPI::sendTrigger(
                m_animation,
                "ToChase"
            );

            Debug::log(
                "[EnemyAttackState] Attack finished, Chase trigger sent"
            );
        }

        return;
    }
}

void EnemyAttackState::OnStateExit()
{
    if (m_paladinUI)
    {
        m_paladinUI->hideBasicAttackUI();
    }

    m_usePaladinAreaAttack = false;

    Debug::log("[EnemyAttackState] EXIT");
}

void EnemyAttackState::lockPaladinAttackArea()
{
    if (!m_controller ||
        !m_controller->getAttackConfig() ||
        !m_committedTarget)
    {
        return;
    }

    Transform* ownerTransform =
        GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return;
    }

    const Vector3 ownerPosition =
        TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 targetPosition =
        TransformAPI::getGlobalPosition(m_committedTarget);

    m_lockedTargetPosition = targetPosition;

    Vector3 direction =
        targetPosition - ownerPosition;

    direction.y = 0.0f;

    if (direction.LengthSquared() < 0.0001f)
    {
        return;
    }

    direction.Normalize();

    m_controller->facePosition(m_lockedTargetPosition);

    m_lockedAttackDirection = direction;

    m_lockedAttackOrigin =
        ownerPosition +
        direction * PaladinBasicAttackForwardOffset;

    const EnemyBaseAttackConfig* attackConfig =
        m_controller->getAttackConfig();

    const Vector3 telegraphCenter =
        m_lockedAttackOrigin +
        direction *
        (attackConfig->m_basicAttackRange * 0.5f);

    if (m_paladinUI)
    {
        m_paladinUI->setupBasicAttackUI(
            PaladinBasicAttackWidth,
            attackConfig->m_basicAttackRange
        );

        m_paladinUI->showBasicAttackUI(
            telegraphCenter,
            m_lockedAttackDirection
        );
    }

    if (m_paladinVFX)
    {
        m_paladinVFX->playShieldAttackStart(telegraphCenter, m_lockedAttackDirection);
    }

    m_usePaladinAreaAttack = true;
}

void EnemyAttackState::drawPaladinAttackTelegraph() const
{
    if (!m_controller ||
        !m_controller->getAttackConfig())
    {
        return;
    }

    Vector3 forward = m_lockedAttackDirection;
    forward.y = 0.0f;

    if (forward.LengthSquared() < 0.0001f)
    {
        return;
    }

    forward.Normalize();

    Vector3 right(
        -forward.z,
        0.0f,
        forward.x
    );

    if (right.LengthSquared() < 0.0001f)
    {
        return;
    }

    right.Normalize();

    const EnemyBaseAttackConfig* attackConfig =
        m_controller->getAttackConfig();

    const float halfWidth =
        PaladinBasicAttackWidth * 0.5f;

    Vector3 startCenter = m_lockedAttackOrigin;
    startCenter.y += 0.05f;

    const Vector3 endCenter =
        startCenter +
        forward * attackConfig->m_basicAttackRange;

    const Vector3 leftStart =
        startCenter -
        right * halfWidth;

    const Vector3 rightStart =
        startCenter +
        right * halfWidth;

    const Vector3 leftEnd =
        endCenter -
        right * halfWidth;

    const Vector3 rightEnd =
        endCenter +
        right * halfWidth;

    const Vector3 telegraphColor(
        1.0f,
        0.25f,
        0.1f
    );

    DebugDrawAPI::drawLine(
        leftStart,
        leftEnd,
        telegraphColor,
        0,
        true
    );

    DebugDrawAPI::drawLine(
        rightStart,
        rightEnd,
        telegraphColor,
        0,
        true
    );

    DebugDrawAPI::drawLine(
        leftStart,
        rightStart,
        telegraphColor,
        0,
        true
    );

    DebugDrawAPI::drawLine(
        leftEnd,
        rightEnd,
        telegraphColor,
        0,
        true
    );
}

void EnemyAttackState::applyPaladinAreaDamage()
{
    if (!m_controller ||
        !m_controller->getAttackConfig() ||
        !m_attackExecutor)
    {
        return;
    }

    const EnemyBaseAttackConfig* attackConfig =
        m_controller->getAttackConfig();

    m_attackExecutor->applyDamageInRectangle(
        m_lockedAttackOrigin,
        m_lockedAttackDirection,
        attackConfig->m_basicAttackRange,
        PaladinBasicAttackWidth,
        attackConfig->m_basicAttackDamage,
        "PaladinBasicAttack"
    );
}

void EnemyAttackState::tryDamageTarget(
    Transform* targetTransform
)
{
    if (!m_controller ||
        !m_controller->getAttackConfig() ||
        !targetTransform)
    {
        return;
    }

    GameObject* targetObject =
        ComponentAPI::getOwner(targetTransform);

    if (!targetObject)
    {
        return;
    }

    PlayerState* playerState =
        GameObjectAPI::findScript<PlayerState>(targetObject);

    if (playerState && playerState->isDowned())
    {
        return;
    }

    Damageable* damageable =
        GameObjectAPI::findScript<Damageable>(targetObject);

    if (!damageable)
    {
        return;
    }

    const EnemyBaseAttackConfig* attackConfig =
        m_controller->getAttackConfig();

    if (m_attackExecutor)
    {
        m_attackExecutor->damageTarget(
            targetTransform,
            attackConfig->m_basicAttackDamage,
            "EnemyBasicAttack"
        );
        return;
    }

    damageable->takeDamage(
        attackConfig->m_basicAttackDamage
    );

    Debug::log(
        "[EnemyAttackState] Damaged '%s' for %.2f.",
        GameObjectAPI::getName(targetObject),
        attackConfig->m_basicAttackDamage
    );
}

void EnemyAttackState::playBasicAttackEffect()
{
    if (m_paladinVFX)
    {
        m_paladinVFX->playBasicAttackEffect();
    }
}

IMPLEMENT_SCRIPT(EnemyAttackState)