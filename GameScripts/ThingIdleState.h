#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class MovingThingController;
class AnimationComponent;

class ThingIdleState : public StateMachineScript
{
    DECLARE_SCRIPT(ThingIdleState)

public:
    explicit ThingIdleState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:

	Transform* firstPlayerInRange() const; // This might not be precise, but it checks both players and returns the first one in range, or nullptr if none are in range.
	bool isPlayerInRange(Transform* playerTransform, float detectionRadiusSquared) const;

	Vector2 findFleeDirection(Transform* playerTransform) const;

    MovingThingController* m_controller = nullptr;
    AnimationComponent* m_animation = nullptr;
};

