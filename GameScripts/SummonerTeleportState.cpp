#include "pch.h"
#include "SummonerTeleportState.h"

#include "SummonerEnemyController.h"
#include "SummonerParticles.h"

SummonerTeleportState::SummonerTeleportState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerTeleportState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_particles = GameObjectAPI::findScript<SummonerParticles>(getOwner());

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

	if (m_controller->isForcedMovementActive())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	Vector3 departPosition = Vector3::Zero;

	if (ownerTransform)
	{
		departPosition = TransformAPI::getGlobalPosition(ownerTransform);
	}

	if (m_particles)
	{
		m_particles->playTeleportParticle(departPosition);
	}

	Vector3 teleportPosition;
	if (m_controller->tryGetTeleportPosition(teleportPosition))
	{
		if (ownerTransform)
		{
			teleportPosition.y = departPosition.y;
			TransformAPI::setGlobalPosition(ownerTransform, teleportPosition);
			m_controller->consumeTeleportCooldown();

			if (m_particles)
			{
				m_particles->playTeleportParticle(teleportPosition);
			}

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
