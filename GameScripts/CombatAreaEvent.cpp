#include "pch.h"
#include "CombatAreaEvent.h"

#include "GameplayEventTrigger.h"
#include "Damageable.h"
#include "ObjectVfxIds.h"

IMPLEMENT_SCRIPT_FIELDS(CombatAreaEvent,
    SERIALIZED_COMPONENT_REF_VECTOR(m_enemies, "Enemies", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_entranceBlocker, "Entrance Blocker", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_exitBlocker, "Exit Blocker", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_entranceVisuals, "Entrance Visuals", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_exitVisuals, "Exit Visuals", ComponentType::TRANSFORM)
)

CombatAreaEvent::CombatAreaEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
    m_isPersistent = true;
}

void CombatAreaEvent::Update()
{
    m_timedParticles.update(Time::getDeltaTime());

    if (!m_isActive || m_hasCompleted)
    {
        return;
    }

    removeDeadEnemies();

    if (!m_remainingEnemies.empty())
    {
        return;
    }

    openArea();

    m_isActive = false;
    m_hasCompleted = true;
}

void CombatAreaEvent::OnGameStop()
{
    ParticleLifecycle::destroy(m_entranceBarricadeVfx.mistInstance);
    ParticleLifecycle::destroy(m_entranceBarricadeVfx.burstInstance);
    ParticleLifecycle::destroy(m_exitBarricadeVfx.mistInstance);
    ParticleLifecycle::destroy(m_exitBarricadeVfx.burstInstance);
    m_timedParticles.clear();
}

void CombatAreaEvent::executeEvent(GameplayEventTrigger* trigger)
{
    if (m_hasCompleted)
    {
        return;
    }

    m_remainingEnemies = m_enemies;

    closeArea();

    m_isActive = true;
}

void CombatAreaEvent::closeArea()
{
    setBlockerState(m_entranceBlocker, true);
    setBlockerState(m_exitBlocker, true);
    activateBarricadeVisuals(m_entranceVisuals, m_entranceBarricadeVfx);
    activateBarricadeVisuals(m_exitVisuals, m_exitBarricadeVfx);
}

void CombatAreaEvent::openArea()
{
    setBlockerState(m_entranceBlocker, false);
    setBlockerState(m_exitBlocker, false);
    deactivateBarricadeVisuals(m_entranceBarricadeVfx);
    deactivateBarricadeVisuals(m_exitBarricadeVfx);
}

void CombatAreaEvent::setBlockerState(const ComponentRef<Transform>& blockerTransformRef, bool blocked)
{
    Transform* blockerTransform = blockerTransformRef.getReferencedComponent();

    if (blockerTransform == nullptr)
    {
        Debug::warn("CombatAreaEvent on '%s' has a missing blocker reference.", GameObjectAPI::getName(getOwner()));
        return;
    }

    GameObject* blockerObject = ComponentAPI::getOwner(blockerTransform);

    if (blockerObject == nullptr)
    {
        return;
    }

    auto* runtimeBlocker = NavigationAPI::getRuntimeBlockerComponent(blockerObject);

    if (runtimeBlocker == nullptr)
    {
        Debug::warn("CombatAreaEvent on '%s' has blocker object '%s' without NavRuntimeBlocker.", GameObjectAPI::getName(getOwner()), GameObjectAPI::getName(blockerObject));
        return;
    }

    NavigationAPI::setBlocked(runtimeBlocker, blocked);
}

void CombatAreaEvent::activateBarricadeVisuals(const ComponentRef<Transform>& visualsTransformRef, BarricadeVisualSlot& slot)
{
    Transform* visualsTransform = visualsTransformRef.getReferencedComponent();
    if (visualsTransform == nullptr)
    {
        Debug::warn("CombatAreaEvent on '%s' has a missing visuals reference.", GameObjectAPI::getName(getOwner()));
        return;
    }

    GameObject* visualsObject = ComponentAPI::getOwner(visualsTransform);
    if (visualsObject != nullptr)
    {
        GameObjectAPI::setActive(visualsObject, false);
    }

    const Vector3 position = TransformAPI::getGlobalPosition(visualsTransform);
    const Vector3 rotation = TransformAPI::getGlobalEulerDegrees(visualsTransform);

    ParticleLifecycle::ensurePersistent(
        slot.mistInstance,
        ObjectVfxIds::barricadeMist(),
        position,
        rotation,
        nullptr
    );

    if (slot.mistInstance != nullptr)
    {
        Transform* mistTransform = GameObjectAPI::getTransform(slot.mistInstance);
        if (mistTransform != nullptr)
        {
            TransformAPI::setGlobalPosition(mistTransform, position);
            TransformAPI::setGlobalRotationEuler(mistTransform, rotation);
        }

        ParticleLifecycle::activate(slot.mistInstance);
    }

    if (slot.burstInstance == nullptr)
    {
        ParticleLifecycle::ensurePersistent(
            slot.burstInstance,
            ObjectVfxIds::barricadeBurst(),
            position,
            rotation,
            nullptr
        );
    }
    else
    {
        Transform* burstTransform = GameObjectAPI::getTransform(slot.burstInstance);
        if (burstTransform != nullptr)
        {
            TransformAPI::setGlobalPosition(burstTransform, position);
            TransformAPI::setGlobalRotationEuler(burstTransform, rotation);
        }
    }

    if (slot.burstInstance != nullptr)
    {
        ParticleLifecycle::activateTimed(m_timedParticles, slot.burstInstance, kBarricadeBurstDuration);
    }
}

void CombatAreaEvent::deactivateBarricadeVisuals(BarricadeVisualSlot& slot)
{
    ParticleLifecycle::deactivate(slot.mistInstance);

    if (slot.burstInstance != nullptr)
    {
        ParticleLifecycle::deactivate(slot.burstInstance);
    }
}

void CombatAreaEvent::removeDeadEnemies()
{
    for (auto it = m_remainingEnemies.begin(); it != m_remainingEnemies.end();)
    {
        if (shouldRemoveEnemy(*it))
        {
            it = m_remainingEnemies.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool CombatAreaEvent::shouldRemoveEnemy(const ComponentRef<Transform>& enemyTransformRef) const
{
    Transform* enemyTransform = enemyTransformRef.getReferencedComponent();

    if (enemyTransform == nullptr)
    {
        return true;
    }

    GameObject* enemyObject = ComponentAPI::getOwner(enemyTransform);

    if (enemyObject == nullptr)
    {
        return true;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(enemyObject);

    if (damageable == nullptr)
    {
        Debug::warn("CombatAreaEvent on '%s' has enemy '%s' without Damageable.", GameObjectAPI::getName(getOwner()), GameObjectAPI::getName(enemyObject));

        return true;
    }

    return damageable->isDead();
}

IMPLEMENT_SCRIPT(CombatAreaEvent)
