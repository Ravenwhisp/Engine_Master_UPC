#include "pch.h"
#include "BreakableDamageable.h"
#include "BreakableObject.h"

#include "PersistingCheckpointState.h"

BreakableDamageable::BreakableDamageable(GameObject* owner)
    : Damageable(owner)
{
}

void BreakableDamageable::Start()
{
    if(!PersistingCheckpointState::Get().IsStartOfLevel())
    {
        std::vector<UID>* brokenBreakables = &PersistingCheckpointState::Get().m_brokenBreakablesPersistent;

        if (std::find(brokenBreakables->begin(), brokenBreakables->end(), m_owner->GetID()) != brokenBreakables->end())
        {
            GameObjectAPI::removeGameObject(m_owner);
            return;
        }
    }

    Damageable::Start();

	m_breakableObject = GameObjectAPI::findScript<BreakableObject>(getOwner());

    if (m_breakableObject == nullptr)
    {
        Debug::warn("[BreakableDamageable] '%s' has no BreakableObject script.", GameObjectAPI::getName(getOwner()));
    }
}

void BreakableDamageable::onDeath()
{
    Damageable::onDeath();

    if (m_breakableObject != nullptr)
    {
        m_breakableObject->onBreak();
    }

    PersistingCheckpointState::Get().m_brokenBreakables.push_back(m_owner->GetID());
}

IMPLEMENT_SCRIPT(BreakableDamageable)