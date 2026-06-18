#include "pch.h"
#include "ArcherSomersaultState.h"

#include "RangedEnemyController.h"
#include "ArcherAttackConfig.h"

ArcherSomersaultState::ArcherSomersaultState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArcherSomersaultState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArcherAttackConfig>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    m_stateTimer = 0.0f;
    m_escapeDirection = Vector3(0.0f, 0.0f, 0.0f);

    if (!m_archerController)
    {
        Debug::error("[ArcherSomersaultState] RangedEnemyController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArcherSomersaultState] ArcherAttackConfig not found.");
        return;
    }
    if (!m_animation)
    {
        Debug::error("[ArcherSomersaultState] AnimationComponent not found.");
        return;
    }

    m_archerController->clearPath();
    m_archerController->resetRepathTimer();

    m_escapeDirection = m_archerController->getDirectionAwayFromClosestPlayer();

    Debug::log("[ArcherSomersaultState] ENTER");
}

void ArcherSomersaultState::OnStateUpdate()
{
    if (!m_archerController || !m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_archerController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_archerController->trySendStunTrigger(m_animation))
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    moveSomersault();

    if (m_stateTimer >= m_attackConfig->m_somersaultDuration)
    {
        finishSomersault();
        return;
    }
}

void ArcherSomersaultState::OnStateExit()
{
    Debug::log("[ArcherSomersaultState] EXIT");
}

void ArcherSomersaultState::moveSomersault()
{
    if (!m_attackConfig)
    {
        return;
    }

    if (m_escapeDirection.LengthSquared() <= 0.00001f)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    const float duration = m_attackConfig->m_somersaultDuration;
    if (duration <= 0.0f)
    {
        return;
    }

    const float speed = m_attackConfig->m_somersaultDistance / duration;
    const float stepDistance = speed * Time::getDeltaTime();

    Vector3 currentPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 desiredPosition = currentPosition + m_escapeDirection * stepDistance;

    Vector3 nextPosition;
    if (NavigationAPI::moveAlongSurface(currentPosition, desiredPosition, nextPosition, Vector3(5.0f, 5.0f, 5.0f)))
    {
        TransformAPI::setPosition(ownerTransform, nextPosition);
    }
}

void ArcherSomersaultState::finishSomersault()
{
    if (!m_archerController || !m_animation)
    {
        return;
    }

    m_archerController->consumeSomersaultCooldown();

    AnimationAPI::sendTrigger(m_animation, "ToChase");

    Debug::log("[ArcherSomersaultState] Finished, Chase trigger sent");
}

IMPLEMENT_SCRIPT(ArcherSomersaultState)