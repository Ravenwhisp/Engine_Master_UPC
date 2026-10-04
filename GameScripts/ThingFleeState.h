#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class MovingThingController;
class AnimationComponent;

class ThingFleeState :public StateMachineScript
{
    DECLARE_SCRIPT(ThingFleeState)

public:
    explicit ThingFleeState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:

	bool tryMoveFleeDirection();

    MovingThingController* m_controller = nullptr;
    AnimationComponent* m_animation = nullptr;

	//bool m_wallDetected = false;
};

