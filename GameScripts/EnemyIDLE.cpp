#include "pch.h"
#include "EnemyIDLE.h"

static const ScriptFieldInfo IDLEFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(EnemyIDLE, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyIDLE, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyIDLE, IDLEFields)


EnemyIDLE::EnemyIDLE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyIDLE::OnStateEnter()
{
}

void EnemyIDLE::OnStateUpdate()
{
}

void EnemyIDLE::OnStateExit()
{
}

IMPLEMENT_SCRIPT(EnemyIDLE)