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
    bool isTriggerRegistered(TriggerComponent* trigger) const;

private:
    std::vector<TriggerComponent*> m_triggers;
};