#include "pch.h"
#include "EnemyCHASE.h"

static const ScriptFieldInfo CHASEFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(EnemyCHASE, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyCHASE, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyCHASE, CHASEFields)

EnemyCHASE::EnemyCHASE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyCHASE::OnStateEnter()
{
}

void EnemyCHASE::OnStateUpdate()
{
}

void EnemyCHASE::OnStateExit()
{
}

IMPLEMENT_SCRIPT(EnemyCHASE)