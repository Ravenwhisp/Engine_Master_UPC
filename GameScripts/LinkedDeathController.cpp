#include "pch.h"
#include "LinkedDeathController.h"
#include "EnemyDeathState.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(LinkedDeathController,
	SERIALIZED_COMPONENT_REF(m_linkedPartner, "Linked Partner", ComponentType::TRANSFORM),
	SERIALIZED_FLOAT(m_graceWindow, "Grace Window", 0.0f, 30.0f, 0.1f),
	SERIALIZED_FLOAT(m_reviveHpPercent, "Revive HP %", 0.0f, 1.0f, 0.05f)
)

LinkedDeathController::LinkedDeathController(GameObject* owner)
	: Script(owner)
{
}

void LinkedDeathController::Start()
{
	m_deathState = GameObjectAPI::findScript<EnemyDeathState>(getOwner());
}

void LinkedDeathController::Update()
{
	switch (m_state)
	{
	case State::Inactive:
	{
		if (m_deathState != nullptr && m_deathState->isDeathActive())
		{
			m_deathState->pauseDeathCountdown();
			m_state = State::EnteringDeath;
		}
		break;
	}
	case State::EnteringDeath:
	{
		findPartnerController();

		if (m_partnerController == nullptr || isPartnerDead() || m_partnerDiedNotification)
		{
			m_state = State::ProceedingToDeath;
			m_deathState->finalizeDeathNow();
			return;
		}

		m_partnerDiedNotification = false;
		m_state = State::Waiting;
		m_pendingDeathTimer = m_graceWindow;

		m_partnerController->notifyLinkedDeath();

		Debug::log("[LinkedDeath] %s died first. Waiting %.1fs for partner.",
			GameObjectAPI::getName(getOwner()), m_graceWindow);
		break;
	}
	case State::Waiting:
	{
		if (isPartnerDead())
		{
			m_partnerDiedNotification = false;
			m_state = State::ProceedingToDeath;

			Debug::log("[LinkedDeath] %s: partner died in time. Proceeding with death.",
				GameObjectAPI::getName(getOwner()));

			m_deathState->finalizeDeathNow();
			return;
		}

		m_pendingDeathTimer -= Time::getDeltaTime();
		if (m_pendingDeathTimer <= 0.0f)
		{
			Debug::log("[LinkedDeath] %s: grace window expired. Reviving.",
				GameObjectAPI::getName(getOwner()));
			reviveAndExit();
		}
		break;
	}
	case State::ProceedingToDeath:
		break;
	case State::Reviving:
	{
		if (m_deathState != nullptr && !m_deathState->isDeathActive())
		{
			m_state = State::Inactive;
		}
		break;
	}
	}
}

void LinkedDeathController::notifyLinkedDeath()
{
	m_partnerDiedNotification = true;

	if (m_state == State::Waiting)
	{
		m_state = State::ProceedingToDeath;
		m_deathState->finalizeDeathNow();
	}
}

void LinkedDeathController::findPartnerController()
{
	Transform* partnerTransform = m_linkedPartner.getReferencedComponent();
	if (partnerTransform == nullptr)
	{
		Debug::warn("LinkedDeathController on '%s' is missing Linked Partner.",
			GameObjectAPI::getName(getOwner()));
		return;
	}

	GameObject* partnerObject = ComponentAPI::getOwner(partnerTransform);
	if (partnerObject == nullptr)
	{
		Debug::warn("LinkedDeathController on '%s' has invalid Linked Partner owner.",
			GameObjectAPI::getName(getOwner()));
		return;
	}

	m_partnerController = GameObjectAPI::findScript<LinkedDeathController>(partnerObject);
	if (m_partnerController == nullptr)
	{
		Debug::warn("LinkedDeathController on '%s' could not find LinkedDeathController on partner.",
			GameObjectAPI::getName(getOwner()));
	}
}

bool LinkedDeathController::isPartnerDead() const
{
	if (m_partnerController == nullptr)
	{
		return false;
	}

	Damageable* partnerDamageable = GameObjectAPI::findScript<Damageable>(
		m_partnerController->getOwner());
	return partnerDamageable != nullptr && partnerDamageable->isDead();
}

void LinkedDeathController::reviveAndExit()
{
	m_state = State::Reviving;
	m_pendingDeathTimer = 0.0f;

	if (m_partnerController != nullptr)
	{
		m_partnerController->m_partnerDiedNotification = false;
	}

	if (m_deathState != nullptr)
	{
		m_deathState->abortDeathForRevival();
	}

	Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());
	if (damageable != nullptr)
	{
		damageable->revive(damageable->getMaxHp() * m_reviveHpPercent);
	}

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (animation != nullptr)
	{
		AnimationAPI::playState(animation, "Idle");
	}
}

IMPLEMENT_SCRIPT(LinkedDeathController)
