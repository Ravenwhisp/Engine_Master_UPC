#include "pch.h"
#include "ThingIdleState.h"

#include "MovingThingController.h"

ThingIdleState::ThingIdleState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ThingIdleState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<MovingThingController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_controller)
    {
        Debug::error("[ThingIdleState] MovingThingController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ThingIdleState] AnimationComponent not found.");
        return;
    }

    m_controller->setIsOnIdle(true);

    Debug::log("[ThingIdleState] ENTER");
}

void ThingIdleState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (!m_controller->getDeathTransform() || !m_controller->getLyrielTransform())
    {
        return;
    }

	Transform* playerInRange = firstPlayerInRange();

    if (playerInRange)
    {
        m_controller->setFleeDirection(findFleeDirection(playerInRange));
        AnimationAPI::sendTrigger(m_animation, "ToFlee");

        Debug::log("[ThingIdleState] Flee trigger sent");
	}
}

Transform* ThingIdleState::firstPlayerInRange() const
{
    Transform* lyrielTransform = m_controller->getLyrielTransform();
    Transform* deathTransform = m_controller->getDeathTransform();

    const float detectionDistanceSquared = m_controller->getDetectionRadius() * m_controller->getDetectionRadius();

    if (isPlayerInRange(lyrielTransform, detectionDistanceSquared))
    {
        return lyrielTransform;
    }

    if (isPlayerInRange(deathTransform, detectionDistanceSquared))
    {
        return deathTransform;
    }

	return nullptr;
}

bool ThingIdleState::isPlayerInRange(Transform* playerTransform, float detectionRadiusSquared) const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    
    if (!ownerTransform)
    {
        return false;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getGlobalPosition(playerTransform);

    Vector3 difference = targetPosition - ownerPosition;
    difference.y = 0.0f;

    const float distanceToTargetSquared = difference.LengthSquared();

    return distanceToTargetSquared <= detectionRadiusSquared;
}

Vector2 ThingIdleState::findFleeDirection(Transform* playerTransform) const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 fleePosition = TransformAPI::getGlobalPosition(playerTransform);

	Vector3 fleeDirection3D = ownerPosition - fleePosition;
	Vector2 fleeDirection2D(fleeDirection3D.x, fleeDirection3D.z);
	fleeDirection2D.Normalize();

    return fleeDirection2D;
}

void ThingIdleState::OnStateExit()
{
    m_controller->setIsOnIdle(false);

    Debug::log("[ThingIdleState] EXIT");
}

IMPLEMENT_SCRIPT(ThingIdleState)