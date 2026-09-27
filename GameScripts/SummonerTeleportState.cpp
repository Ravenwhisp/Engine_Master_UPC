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

	if (m_controller->trySendStunTrigger(m_animation))
	{
		return;
	}

	Debug::log("[SummonerTeleportState] ENTER");

	if (m_controller->isForcedMovementActive())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		Debug::warn("[SummonerTeleportState] Owner transform not found.");
		m_controller->delayTeleportRetry();
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	const Vector3 departPosition = TransformAPI::getGlobalPosition(ownerTransform);

	Vector3 teleportPosition;
	if (m_controller->tryGetTeleportPosition(teleportPosition))
	{
		teleportPosition.y = departPosition.y;

		if (m_particles)
		{
			m_particles->playTeleportParticle(departPosition);
		}

		TransformAPI::setGlobalPosition(ownerTransform, teleportPosition);
		m_controller->consumeTeleportCooldown();

		if (m_particles)
		{
			m_particles->playTeleportParticle(teleportPosition);
		}

		Debug::log("[SummonerTeleportState] Teleported.");
	}
	else
	{
		m_controller->delayTeleportRetry();
		Debug::warn("[SummonerTeleportState] No valid teleport position found.");
	}

	AnimationAPI::sendTrigger(m_animation, "ToIdle");
}

void SummonerTeleportState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	m_controller->trySendStunTrigger(m_animation);
}

void SummonerTeleportState::OnStateExit()
{
	Debug::log("[SummonerTeleportState] EXIT");
}

IMPLEMENT_SCRIPT(SummonerTeleportState)
