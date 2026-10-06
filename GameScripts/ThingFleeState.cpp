#include "pch.h"
#include "ThingFleeState.h"

#include "MovingThingController.h"

ThingFleeState::ThingFleeState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ThingFleeState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<MovingThingController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_controller)
    {
        Debug::error("[ThingFleeState] MovingThingController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ThingFleeState] AnimationComponent not found.");
        return;
    }

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector2 fleeDirection = m_controller->getFleeDirection();

    TransformAPI::lookAt(ownerTransform, Vector3(ownerPosition.x + fleeDirection.x, ownerPosition.y, ownerPosition.z + fleeDirection.y) );

    Debug::log("[ThingFleeState] ENTER");
}

void ThingFleeState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (!tryMoveFleeDirection())
    {
        AnimationAPI::sendTrigger(m_animation, "ToEnd");

        Debug::log("[ThingFleeState] End trigger sent");
        return;
    }

    /*
    // m_wallDetected here would enable the upper if, instead of using tryMoveFleeDirection() directly
    if (!tryMoveFleeDirection())
    {
        m_wallDetected = true;
        Debug::log("[ThingFleeState] Wall detected, stopping movement");
	}
    */
}

void ThingFleeState::OnStateExit()
{
	GameObject* owner = getOwner();
	Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    GameObjectAPI::instantiatePrefab(m_controller->getDisappearParticlesId(), TransformAPI::getGlobalPosition(ownerTransform), Vector3(0.f, 0.f, 0.f));
	GameObjectAPI::removeGameObject(owner); // Will need to be extended to add particles and sound effects for the death of the thing.

    Debug::log("[ThingFleeState] EXIT");
}

bool ThingFleeState::tryMoveFleeDirection()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

	Vector2 fleeDirection = m_controller->getFleeDirection();

	float lengthMovement = m_controller->getFleeSpeed() * Time::getDeltaTime();
	Vector3 desiredPosition = ownerPosition + Vector3(fleeDirection.x, 0.0f, fleeDirection.y) * lengthMovement;

	Vector3 nextPosition;

    if (NavigationAPI::moveAlongSurface(ownerPosition, desiredPosition, nextPosition, Vector3(5.0f, 5.0f, 5.0f)))
    {
		Vector3 movementDelta = nextPosition - ownerPosition;

		TransformAPI::setGlobalPosition(ownerTransform, nextPosition); // Move the object to the new position

        if (movementDelta.LengthSquared() < (lengthMovement * (lengthMovement / 2.f)) )
        {
            return false;
        }

        return true;
	}

    return false;
}

IMPLEMENT_SCRIPT(ThingFleeState)