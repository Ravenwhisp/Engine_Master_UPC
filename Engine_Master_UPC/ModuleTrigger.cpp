#include "Globals.h"
#include "ModuleTrigger.h"

#include "Application.h"
#include "GameObject.h"
#include "TriggerComponent.h"
#include "BoundingBox.h"

void ModuleTrigger::update()
{
    if (app->getCurrentEngineState() != ENGINE_STATE::PLAYING)
    {
        return;
    }

    detectOverlaps();
    processOverlapChanges();
}

bool ModuleTrigger::cleanUp()
{
    m_triggers.clear();
    m_previousOverlaps.clear();
    m_currentOverlaps.clear();

    return true;
}

void ModuleTrigger::registerTrigger(TriggerComponent* trigger)
{
    if (!trigger)
    {
        return;
    }

    if (isTriggerRegistered(trigger))
    {
        return;
    }

    m_triggers.push_back(trigger);

    DEBUG_LOG( "[ModuleTrigger] Registered trigger %llu", static_cast<unsigned long long>(trigger->getID()));
}

void ModuleTrigger::unregisterTrigger(TriggerComponent* trigger)
{
    if(!trigger)
    {
        return;
    }

    auto it = std::find(m_triggers.begin(), m_triggers.end(), trigger);

    if (it == m_triggers.end())
    {
        return;
    }

    const UID triggerId = trigger->getID();

    m_triggers.erase(it);
    removeOverlaps(triggerId);

    DEBUG_LOG( "[ModuleTrigger] Unregistered trigger %llu", static_cast<unsigned long long>(trigger->getID()));
}

void ModuleTrigger::detectOverlaps()
{
    m_previousOverlaps = m_currentOverlaps;
    m_currentOverlaps.clear();

    for (size_t i = 0; i < m_triggers.size(); ++i)
    {
        TriggerComponent* triggerA = m_triggers[i];

        if (!isValidTrigger(triggerA))
        {
            continue;
        }

        for (size_t j = i + 1; j < m_triggers.size(); ++j)
        {
            TriggerComponent* triggerB = m_triggers[j];

            if (!isValidTrigger(triggerB))
            {
                continue;
            }

            if (triggerA->getOwner() == triggerB->getOwner())
            {
                continue;
            }

            if (intersectsAABB(triggerA, triggerB))
            {
                m_currentOverlaps.push_back(TriggerOverlap(triggerA->getID(), triggerB->getID()));
            }
        }
    }
}

void ModuleTrigger::processOverlapChanges()
{
    for (const TriggerOverlap& overlap : m_currentOverlaps)
    {
        if (!containsOverlap(m_previousOverlaps, overlap))
        {
            DEBUG_LOG("[ModuleTrigger] Trigger Enter");
        }
    }

    for (const TriggerOverlap& overlap : m_previousOverlaps)
    {
        if (!containsOverlap(m_currentOverlaps, overlap))
        {
            DEBUG_LOG("[ModuleTrigger] Trigger Exit");
        }
    }
}

void ModuleTrigger::removeOverlaps(UID triggerId)
{
    auto removeOverlap = [triggerId](const TriggerOverlap& overlap)
        {
            return overlap.a == triggerId || overlap.b == triggerId;
        };

    m_previousOverlaps.erase(std::remove_if(m_previousOverlaps.begin(), m_previousOverlaps.end(), removeOverlap), m_previousOverlaps.end());

    m_currentOverlaps.erase(std::remove_if(m_currentOverlaps.begin(), m_currentOverlaps.end(), removeOverlap), m_currentOverlaps.end());
}

bool ModuleTrigger::isTriggerRegistered(TriggerComponent* trigger) const
{
    return std::find(m_triggers.begin(), m_triggers.end(), trigger) != m_triggers.end();
}

bool ModuleTrigger::isValidTrigger(TriggerComponent* trigger) const
{
    if (!trigger || !trigger->isActive())
    {
        return false;
    }

    GameObject* owner = trigger->getOwner();

    if (!owner || !owner->IsActiveInWindowHierarchy())
    {
        return false;
    }

    return true;
}

bool ModuleTrigger::intersectsAABB(TriggerComponent* a, TriggerComponent* b)
{
    Engine::BoundingBox& boxA = a->getWorldAABB();
    Engine::BoundingBox& boxB = b->getWorldAABB();

    const Vector3& aMin = boxA.getMin();
    const Vector3& aMax = boxA.getMax();

    const Vector3& bMin = boxB.getMin();
    const Vector3& bMax = boxB.getMax();

    const bool overlapsX = aMin.x <= bMax.x && aMax.x >= bMin.x;
    const bool overlapsY = aMin.y <= bMax.y && aMax.y >= bMin.y;
    const bool overlapsZ = aMin.z <= bMax.z && aMax.z >= bMin.z;

    return overlapsX && overlapsY && overlapsZ;
}

bool ModuleTrigger::containsOverlap(const std::vector<TriggerOverlap>& overlaps, const TriggerOverlap& overlap) const
{
    return std::find(overlaps.begin(), overlaps.end(), overlap) != overlaps.end();
}