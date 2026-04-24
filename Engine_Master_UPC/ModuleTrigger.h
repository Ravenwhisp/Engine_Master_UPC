#pragma once

#include "Module.h"

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
    void detectOverlaps();

    bool isTriggerRegistered(TriggerComponent* trigger) const;
    bool isValidTrigger(TriggerComponent* trigger) const;
    bool intersectsAABB(TriggerComponent* a, TriggerComponent* b);

private:
    std::vector<TriggerComponent*> m_triggers;
};