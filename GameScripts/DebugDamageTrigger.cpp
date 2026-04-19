#include "pch.h"
#include "DebugDamageTrigger.h"
#include "Damageable.h"

static const ScriptFieldInfo debugDamageTriggerFields[] =
{
    { "Damage Amount", ScriptFieldType::Float, offsetof(DebugDamageTrigger, m_damageAmount), { 0.0f, 999999.0f, 1.0f } },
    { "Heal Amount", ScriptFieldType::Float, offsetof(DebugDamageTrigger, m_healAmount), { 0.0f, 999999.0f, 1.0f } },
    { "Player Index", ScriptFieldType::Int, offsetof(DebugDamageTrigger, m_playerIndex) }
};

IMPLEMENT_SCRIPT_FIELDS(DebugDamageTrigger, debugDamageTriggerFields)

DebugDamageTrigger::DebugDamageTrigger(GameObject* owner)
    : Script(owner)
{
}

void DebugDamageTrigger::Start()
{
    m_damageable = findDamageable();

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

Damageable* DebugDamageTrigger::findDamageable() const
{
    if (!m_owner)
    {
        return nullptr;
    }

    Script* script = GameObjectAPI::getScript(m_owner, "PlayerDamageable");
    Damageable* damageable = dynamic_cast<Damageable*>(script);

    if (damageable)
    {
        return damageable;
    }

    script = GameObjectAPI::getScript(m_owner, "Damageable");
    damageable = dynamic_cast<Damageable*>(script);

    return damageable;
}

IMPLEMENT_SCRIPT(DebugDamageTrigger)