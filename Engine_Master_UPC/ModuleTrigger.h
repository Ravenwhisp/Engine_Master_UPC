#pragma once

#include "Module.h"
#include "UID.h"

#include <vector>

class TriggerComponent;

class ModuleTrigger : public Module
{
public:
    void update() override;
    bool cleanUp() override;

    void registerTrigger(TriggerComponent* trigger);
    void unregisterTrigger(TriggerComponent* trigger);

private:
    struct TriggerOverlap
    {
        UID a = 0;
        UID b = 0;

        TriggerOverlap() = default;

        TriggerOverlap(UID first, UID second)
        {
            if (first < second)
            {
                a = first;
                b = second;
            }
            else
            {
                a = second;
                b = first;
            }
        }

        bool operator==(const TriggerOverlap& other) const
        {
            return a == other.a && b == other.b;
        }
    };

private:
    void detectOverlaps();
    void processOverlapChanges();

    void removeOverlaps(UID triggerId);
    bool isTriggerRegistered(TriggerComponent* trigger) const;
    bool isValidTrigger(TriggerComponent* trigger) const;
    bool intersectsAABB(TriggerComponent* a, TriggerComponent* b);
    bool containsOverlap(const std::vector<TriggerOverlap>& overlaps, const TriggerOverlap& overlap) const;

private:
    std::vector<TriggerComponent*> m_triggers;

    std::vector<TriggerOverlap> m_previousOverlaps;
    std::vector<TriggerOverlap> m_currentOverlaps;
};