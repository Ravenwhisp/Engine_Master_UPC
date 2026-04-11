#include "pch.h"
#include "StateMachineTest.h"

static const ScriptFieldInfo StateMachineTestFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(StateMachineTest, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(StateMachineTest, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(StateMachineTest, StateMachineTestFields)

StateMachineTest::StateMachineTest(GameObject* owner)
    : StateMachineScript(owner)
{
}


void StateMachineTest::OnStateEnter()
{
}

void StateMachineTest::OnStateUpdate()
{
    Debug::log("HOLA");
}

void StateMachineTest::OnStateExit()
{
}

IMPLEMENT_SCRIPT(StateMachineTest)