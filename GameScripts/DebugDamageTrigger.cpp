#include "pch.h"
#include "DebugDamageTrigger.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(DebugDamageTrigger,
    SERIALIZED_FLOAT(m_damageAmount, "Damage Amount", 0.0f, 999999.0f, 1.0f),
    SERIALIZED_FLOAT(m_healAmount, "Heal Amount", 0.0f, 999999.0f, 1.0f),
    SERIALIZED_INT(m_playerIndex, "Player Index")
)

DebugDamageTrigger::DebugDamageTrigger(GameObject* owner)
    : Script(owner)
{
}

void DebugDamageTrigger::Start()
{
    m_damageable = GameObjectAPI::findScript<Damageable>(getOwner());

    if (!m_damageable)
    {
        Debug::warn("DebugDamageTrigger on '%s' could not find a Damageable or PlayerDamageable on the same GameObject.", GameObjectAPI::getName(m_owner));
    }
}

void DebugDamageTrigger::Update()
{
    if (!m_damageable)
    {
        return;
    }

    if (Input::isFaceButtonBottomJustPressed(m_playerIndex))
    {
        m_damageable->takeDamage(m_damageAmount);
    }

    if (Input::isFaceButtonRightJustPressed(m_playerIndex))
    {
        m_damageable->heal(m_healAmount);
    }

    if (Input::isFaceButtonLeftJustPressed(m_playerIndex))
    {
        m_damageable->kill();
    }

    if (Input::isFaceButtonTopJustPressed(m_playerIndex))
    {
        m_damageable->revive();
    }
}

IMPLEMENT_SCRIPT(DebugDamageTrigger)