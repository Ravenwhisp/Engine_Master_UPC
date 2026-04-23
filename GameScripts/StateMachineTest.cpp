#include "pch.h"
#include "StateMachineTest.h"

IMPLEMENT_SCRIPT_FIELDS(StateMachineTest,
    SERIALIZED_FLOAT(m_speed, "Speed", 0.0f, 20.0f, 0.1f),
    SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

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