#pragma once

#include "UID.h"

#include <vector>

class TriggerComponent;
class GameObject;
class Script;

class TriggerSystem
{
public:
    void update();
    void clear();

#pragma region Registration
    void registerTrigger(TriggerComponent* trigger);
    void unregisterTrigger(TriggerComponent* trigger);
#pragma endregion

private:
    // Struct representing an active overlap between two trigger.
    struct TriggerOverlap
    {
        UID firstTriggerId = 0;
        UID secondTriggerId = 0;

        TriggerOverlap() = default;

        TriggerOverlap(UID first, UID second)
        {
            if (first < second)
            {
                firstTriggerId = first;
                secondTriggerId = second;
            }
            else
            {
                firstTriggerId = second;
                secondTriggerId = first;
            }
        }

        bool operator==(const TriggerOverlap& other) const
        {
            return firstTriggerId == other.firstTriggerId && secondTriggerId == other.secondTriggerId;
        }
    };

private:
#pragma region Overlap detection
    void detectOverlaps();
    void processOverlapChanges();
    void removeOverlaps(UID triggerId);
#pragma endregion

#pragma region Script event notification
    void sendTriggerEnterToBothObjects(TriggerComponent* triggerA, TriggerComponent* triggerB);
    void sendTriggerExitToBothObjects(TriggerComponent* triggerA, TriggerComponent* triggerB);

    std::vector<Script*> getActiveScripts(GameObject* gameObject) const;

    void notifyScriptsTriggerEnter(GameObject* receiver, GameObject* other);
    void notifyScriptsTriggerExit(GameObject* receiver, GameObject* other);
#pragma endregion

#pragma region Lookup and validation
    TriggerComponent* findTriggerById(UID triggerId) const;

    bool isTriggerRegistered(TriggerComponent* trigger) const;
    bool isValidTrigger(TriggerComponent* trigger) const;
    bool containsOverlap(const std::vector<TriggerOverlap>& overlaps, const TriggerOverlap& overlap) const;
#pragma endregion

#pragma region Intersection tests
    bool intersectsAABB(TriggerComponent* a, TriggerComponent* b);
#pragma endregion

private:
    std::vector<TriggerComponent*> m_triggers;

    std::vector<TriggerOverlap> m_previousOverlaps;
    std::vector<TriggerOverlap> m_currentOverlaps;
};