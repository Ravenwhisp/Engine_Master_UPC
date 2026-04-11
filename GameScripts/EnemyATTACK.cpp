#include "pch.h"
#include "EnemyATTACK.h"

static const ScriptFieldInfo ATTACKFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(EnemyATTACK, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyATTACK, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyATTACK, ATTACKFields)

EnemyATTACK::EnemyATTACK(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyATTACK::OnStateEnter()
{
}

void EnemyATTACK::OnStateUpdate()
{
}

void EnemyATTACK::OnStateExit()
{
}

IMPLEMENT_SCRIPT(EnemyATTACK)