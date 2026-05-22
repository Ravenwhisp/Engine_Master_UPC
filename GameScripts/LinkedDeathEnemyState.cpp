#include "pch.h"
#include "LinkedDeathEnemyState.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LinkedDeathEnemyState, EnemyDeathState,
    SERIALIZED_COMPONENT_REF(m_linkedPartner, "Linked Partner", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_graceWindow, "Grace Window", 0.0f, 30.0f, 0.1f),
    SERIALIZED_FLOAT(m_reviveHpPercent, "Revive HP %", 0.0f, 1.0f, 0.05f)
)

LinkedDeathEnemyState::LinkedDeathEnemyState(GameObject* owner)
    : EnemyDeathState(owner)
{
}

void LinkedDeathEnemyState::findPartnerHandler()
{
    if (m_initialized)
    {
        return;
    }

    m_initialized = true;

    Transform* partnerTransform = m_linkedPartner.getReferencedComponent();
    if (partnerTransform == nullptr)
    {
        Debug::warn("LinkedDeathEnemyState on '%s' is missing Linked Partner.",
            GameObjectAPI::getName(getOwner()));
        return;
    }

    GameObject* partnerObject = ComponentAPI::getOwner(partnerTransform);
    if (partnerObject == nullptr)
    {
        Debug::warn("LinkedDeathEnemyState on '%s' has invalid Linked Partner owner.",
            GameObjectAPI::getName(getOwner()));
        return;
    }

    m_partnerHandler = GameObjectAPI::findScript<LinkedDeathEnemyState>(partnerObject);
    if (m_partnerHandler == nullptr)
    {
        Debug::warn("LinkedDeathEnemyState on '%s' could not find LinkedDeathEnemyState on partner.",
            GameObjectAPI::getName(getOwner()));
    }
}

bool LinkedDeathEnemyState::isPartnerDead() const
{
    if (m_partnerHandler == nullptr)
    {
        return false;
    }

    Damageable* partnerDamageable = GameObjectAPI::findScript<Damageable>(
        m_partnerHandler->getOwner());
    return partnerDamageable != nullptr && partnerDamageable->isDead();
}

void LinkedDeathEnemyState::notifyLinkedDeath()
{
    m_partnerDiedNotification = true;
}

void LinkedDeathEnemyState::OnStateEnter()
{
    findPartnerHandler();

    onDeathStarted();

    if (m_partnerHandler == nullptr || isPartnerDead() || m_partnerDiedNotification)
    {
        // Partner already dead or no partner → normal death
        EnemyDeathState::OnStateEnter();
        return;
    }

    m_partnerDiedNotification = false;
    m_isWaitingForPartner = true;
    m_pendingDeathTimer = m_graceWindow;

    m_partnerHandler->notifyLinkedDeath();

    Debug::log("[LinkedDeath] %s died first. Waiting %.1fs for partner.",
        GameObjectAPI::getName(getOwner()), m_graceWindow);
}

void LinkedDeathEnemyState::OnStateUpdate()
{
    if (!m_isWaitingForPartner)
    {
        EnemyDeathState::OnStateUpdate();
        return;
    }

    if (isPartnerDead())
    {
        m_isWaitingForPartner = false;
        m_partnerDiedNotification = false;

        Debug::log("[LinkedDeath] %s: partner died in time. Proceeding with death.",
            GameObjectAPI::getName(getOwner()));

        EnemyDeathState::dropRewards();
        EnemyDeathState::startDestroyCountdown(m_destroyDelay);
        return;
    }

    m_pendingDeathTimer -= Time::getDeltaTime();
    if (m_pendingDeathTimer <= 0.0f)
    {
        Debug::log("[LinkedDeath] %s: grace window expired. Reviving.",
            GameObjectAPI::getName(getOwner()));
        reviveAndExit();
    }
}

void LinkedDeathEnemyState::OnStateExit()
{
    m_isWaitingForPartner = false;
    m_pendingDeathTimer = 0.0f;
    m_partnerDiedNotification = false;

    EnemyDeathState::OnStateExit();
}

void LinkedDeathEnemyState::reviveAndExit()
{
    m_isWaitingForPartner = false;
    m_pendingDeathTimer = 0.0f;

    // Notify partner we're alive so their notification flag is cleared
    if (m_partnerHandler != nullptr)
    {
        m_partnerHandler->m_partnerDiedNotification = false;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());
    if (damageable != nullptr)
    {
        damageable->revive(damageable->getMaxHp() * m_reviveHpPercent);
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (animation != nullptr)
    {
        AnimationAPI::playState(animation, "Idle");
    }
}

IMPLEMENT_SCRIPT(LinkedDeathEnemyState)
