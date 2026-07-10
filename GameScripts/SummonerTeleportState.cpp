#include "pch.h"
#include "SummonerTeleportState.h"

#include "SummonerEnemyController.h"

SummonerTeleportState::SummonerTeleportState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerTeleportState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[SummonerTeleportState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SummonerTeleportState] AnimationComponent not found.");
		return;
	}

	Debug::log("[SummonerTeleportState] ENTER");

	Vector3 teleportPosition;
	if (m_controller->tryGetTeleportPosition(teleportPosition))
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

		if (ownerTransform)
		{
			TransformAPI::setPosition(ownerTransform, teleportPosition);
			m_controller->consumeTeleportCooldown();

			Debug::log("[SummonerTeleportState] Teleported.");
		}
	}
	else
	{
		Debug::warn("[SummonerTeleportState] No valid teleport position found.");
	}

	AnimationAPI::sendTrigger(m_animation, "ToIdle");
}

void SummonerTeleportState::OnStateUpdate()
{
}

void SummonerTeleportState::OnStateExit()
{
	Debug::log("[SummonerTeleportState] EXIT");
}

IMPLEMENT_SCRIPT(SummonerTeleportState)