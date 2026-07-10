#include "pch.h"
#include "ArcherSomersaultState.h"

#include "RangedEnemyController.h"
#include "ArcherAttackConfig.h"
#include "ArcherGuardParticles.h"

ArcherSomersaultState::ArcherSomersaultState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArcherSomersaultState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_particles = GameObjectAPI::findScript<ArcherGuardParticles>(getOwner());

    m_stateTimer = 0.0f;
    m_escapeDirection = Vector3(0.0f, 0.0f, 0.0f);

    if (!m_archerController)
    {
        Debug::error("[ArcherSomersaultState] RangedEnemyController not found.");
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

    if (m_particles) m_particles->startChargeParticle();

    Debug::log("[ArcherSomersaultState] ENTER");
}

void ArcherSomersaultState::OnStateUpdate()
{
    if (!m_archerController || !m_animation)
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

    const ArcherAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    if (m_particles) m_particles->updateChargeParticle();
    moveSomersault();

    if (m_stateTimer >= cfg->m_somersaultDuration)
    {
        finishSomersault();
        return;
    }
}

void ArcherSomersaultState::OnStateExit()
{
    if (m_particles) m_particles->stopChargeParticle();
    Debug::log("[ArcherSomersaultState] EXIT");
}

void ArcherSomersaultState::moveSomersault()
{
    const ArcherAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    if (m_escapeDirection.LengthSquared() <= 0.00001f)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    const float duration = cfg->m_somersaultDuration;
    if (duration <= 0.0f)
    {
        return;
    }

    const float speed = cfg->m_somersaultDistance / duration;
    const float stepDistance = speed * Time::getDeltaTime();

    Vector3 currentPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 desiredPosition = currentPosition + m_escapeDirection * stepDistance;

    Vector3 nextPosition;
    if (NavigationAPI::moveAlongSurface(currentPosition, desiredPosition, nextPosition, Vector3(5.0f, 5.0f, 5.0f)))
    {
        TransformAPI::setGlobalPosition(ownerTransform, nextPosition);
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

IMPLEMENT_SCRIPT_FIELDS(ArcherSomersaultState,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)