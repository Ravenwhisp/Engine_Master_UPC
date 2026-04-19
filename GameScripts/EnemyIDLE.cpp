#include "pch.h"
#include "EnemyIDLE.h"
#include "EnemyController.h"

static const ScriptFieldInfo IDLEFields[] =
{
	{ "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyIDLE, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyIDLE, IDLEFields)


EnemyIDLE::EnemyIDLE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyIDLE::OnStateEnter()
{
	Script* script = GameObjectAPI::getScript(getOwner(), "EnemyController");
	m_enemyController = dynamic_cast<EnemyController*>(script);

	if (!m_enemyController)
	{
		return;
	}

	m_enemyController->clearPath();
	m_enemyController->resetRepathTimer();

}

void EnemyIDLE::OnStateUpdate()
{
	if (!m_enemyController)
	{
		return;
	}

	m_enemyController->updateCurrentTarget();

	if (!m_enemyController->hasValidTarget())
	{
		return;
	}

	if (m_enemyController->isTargetInCombatRange())
	{
		m_enemyController->faceCurrentTarget();
		return;
	}

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (!animation)
	{
		return;
	}

	AnimationAPI::playState(animation, "Chase"); // sendTrigger

	if (m_debugEnabled)
	{
		Debug::log("[EnemyIDLE] Chase trigger sent");
	}
}

void EnemyIDLE::OnStateExit()
{
	if (!m_debugEnabled)
	{
		return;
	}

	Debug::log("[EnemyIDLE] EXIT");
}

IMPLEMENT_SCRIPT(EnemyIDLE)