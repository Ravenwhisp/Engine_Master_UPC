#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class SkeletonEnemyController;
class AnimationComponent;

class SkeletonChaseState : public StateMachineScript
{
	DECLARE_SCRIPT(SkeletonChaseState)

public:
	explicit SkeletonChaseState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SkeletonEnemyController* m_skeletonController = nullptr;
	AnimationComponent* m_animation = nullptr;
};