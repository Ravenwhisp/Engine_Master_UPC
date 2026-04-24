#include "Globals.h"
#include "ModuleTrigger.h"

#include "TriggerComponent.h"

void ModuleTrigger::update()
{
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

bool ModuleTrigger::isTriggerRegistered(TriggerComponent* trigger) const
{
    return std::find(m_triggers.begin(), m_triggers.end(), trigger) != m_triggers.end();
}