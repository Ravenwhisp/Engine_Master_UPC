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
}

bool ModuleTrigger::cleanUp()
{
    m_triggers.clear();
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

    m_triggers.erase(it);

    DEBUG_LOG( "[ModuleTrigger] Unregistered trigger %llu", static_cast<unsigned long long>(trigger->getID()));
}

void ModuleTrigger::detectOverlaps()
{
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
                DEBUG_LOG("[ModuleTrigger] Overlap");
            }
        }
    }
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