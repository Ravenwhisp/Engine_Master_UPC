#include "Globals.h"
#include "TriggerSystem.h"

#include "Application.h"
#include "GameObject.h"
#include "TriggerComponent.h"
#include "BoundingBox.h"

#include "ScriptComponent.h"
#include "Script.h"

#include <algorithm>

void TriggerSystem::update()
{
    detectOverlaps();
    processOverlapChanges();
}

void TriggerSystem::clear()
{
    m_triggers.clear();
    m_previousOverlaps.clear();
    m_currentOverlaps.clear();
}

void TriggerSystem::registerTrigger(TriggerComponent* trigger)
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

    DEBUG_LOG("[TrigerSystem] Registered trigger %llu. Total: %zu", static_cast<unsigned long long>(trigger->getID()), m_triggers.size());
}

void TriggerSystem::unregisterTrigger(TriggerComponent* trigger)
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

    DEBUG_LOG("[TrigerSystem] Unregistered trigger %llu. Total: %zu", static_cast<unsigned long long>(triggerId), m_triggers.size());
}

void TriggerSystem::detectOverlaps()
{
    m_previousOverlaps = m_currentOverlaps;
    m_currentOverlaps.clear();

    struct SweepEntry { float minX; float maxX; TriggerComponent* trigger; };

    std::vector<SweepEntry> sweep;
    sweep.reserve(m_triggers.size());
    for (auto* t : m_triggers)
    {
        if (!isValidTrigger(t)) continue;
        const auto& aabb = t->getWorldAABB();
        sweep.push_back({ aabb.getMin().x, aabb.getMax().x, t });
    }
    std::sort(sweep.begin(), sweep.end(),
        [](const auto& a, const auto& b) { return a.minX < b.minX; });

    for (size_t i = 0; i < sweep.size(); ++i)
    {
        TriggerComponent* triggerA = sweep[i].trigger;

        for (size_t j = i + 1; j < sweep.size(); ++j)
        {
            if (sweep[j].minX > sweep[i].maxX) break;

            TriggerComponent* triggerB = sweep[j].trigger;

            if (triggerA->getOwner() == triggerB->getOwner()) continue;

            if (intersectsAABB(triggerA, triggerB))
                m_currentOverlaps.push_back(TriggerOverlap(triggerA->getID(), triggerB->getID()));
        }
    }
}

void TriggerSystem::processOverlapChanges()
{
    for (const TriggerOverlap& overlap : m_currentOverlaps)
    {
        if (!containsOverlap(m_previousOverlaps, overlap))
        {
            TriggerComponent* triggerA = findTriggerById(overlap.firstTriggerId);
            TriggerComponent* triggerB = findTriggerById(overlap.secondTriggerId);

            if (isValidTrigger(triggerA) && isValidTrigger(triggerB))
            {
                sendTriggerEnterToBothObjects(triggerA, triggerB);
            }
        }
    }

    for (const TriggerOverlap& overlap : m_previousOverlaps)
    {
        if (!containsOverlap(m_currentOverlaps, overlap))
        {
            TriggerComponent* triggerA = findTriggerById(overlap.firstTriggerId);
            TriggerComponent* triggerB = findTriggerById(overlap.secondTriggerId);

            if (isValidTrigger(triggerA) && isValidTrigger(triggerB))
            {
                sendTriggerExitToBothObjects(triggerA, triggerB);
            }
        }
    }
}

void TriggerSystem::removeOverlaps(UID triggerId)
{
    auto removeOverlap = [triggerId](const TriggerOverlap& overlap)
        {
            return overlap.firstTriggerId == triggerId || overlap.secondTriggerId == triggerId;
        };

    m_previousOverlaps.erase(std::remove_if(m_previousOverlaps.begin(), m_previousOverlaps.end(), removeOverlap), m_previousOverlaps.end());

    m_currentOverlaps.erase(std::remove_if(m_currentOverlaps.begin(), m_currentOverlaps.end(), removeOverlap), m_currentOverlaps.end());
}

void TriggerSystem::sendTriggerEnterToBothObjects(TriggerComponent* triggerA, TriggerComponent* triggerB)
{
    GameObject* objectA = triggerA->getOwner();
    GameObject* objectB = triggerB->getOwner();

    notifyScriptsTriggerEnter(objectA, objectB);
    notifyScriptsTriggerEnter(objectB, objectA);
}

void TriggerSystem::sendTriggerExitToBothObjects(TriggerComponent* triggerA, TriggerComponent* triggerB)
{
    GameObject* objectA = triggerA->getOwner();
    GameObject* objectB = triggerB->getOwner();

    notifyScriptsTriggerExit(objectA, objectB);
    notifyScriptsTriggerExit(objectB, objectA);
}

std::vector<Script*> TriggerSystem::getActiveScripts(GameObject* gameObject) const
{
    std::vector<Script*> scripts;

    if (!gameObject)
    {
        return scripts;
    }

    const std::vector<Component*> components = gameObject->GetAllComponents();

    for (Component* component : components)
    {
        if (!component || !component->isActive())
        {
            continue;
        }

        if (component->getType() != ComponentType::SCRIPT)
        {
            continue;
        }

        ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(component);
        Script* script = scriptComponent->getScript();

        if (script)
        {
            scripts.push_back(script);
        }
    }

    return scripts;
}

void TriggerSystem::notifyScriptsTriggerEnter(GameObject* receiver, GameObject* other)
{
    if (!receiver || !other)
    {
        return;
    }

    for (Script* script : getActiveScripts(receiver))
    {
        script->OnTriggerEnter(other);
    }
}

void TriggerSystem::notifyScriptsTriggerExit(GameObject* receiver, GameObject* other)
{
    if (!receiver || !other)
    {
        return;
    }

    for (Script* script : getActiveScripts(receiver))
    {
        script->OnTriggerExit(other);
    }
}

TriggerComponent* TriggerSystem::findTriggerById(UID triggerId) const
{
    for (TriggerComponent* trigger : m_triggers)
    {
        if (trigger && trigger->getID() == triggerId)
        {
            return trigger;
        }
    }

    return nullptr;
}

bool TriggerSystem::isTriggerRegistered(TriggerComponent* trigger) const
{
    return std::find(m_triggers.begin(), m_triggers.end(), trigger) != m_triggers.end();
}

bool TriggerSystem::isValidTrigger(TriggerComponent* trigger) const
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

bool TriggerSystem::containsOverlap(const std::vector<TriggerOverlap>& overlaps, const TriggerOverlap& overlap) const
{
    return std::find(overlaps.begin(), overlaps.end(), overlap) != overlaps.end();
}

bool TriggerSystem::intersectsAABB(TriggerComponent* a, TriggerComponent* b)
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