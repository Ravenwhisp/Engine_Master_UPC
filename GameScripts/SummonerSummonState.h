#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

#include <vector>

class SummonerEnemyController;
class AnimationComponent;

class SummonerSummonState : public StateMachineScript
{
	DECLARE_SCRIPT(SummonerSummonState)

public:
	explicit SummonerSummonState(GameObject* owner);

	void OnStateEnter() override;
	void OnStateUpdate() override;
	void OnStateExit() override;

private:
	SummonerEnemyController* m_controller = nullptr;
	AnimationComponent* m_animation = nullptr;
	class SummonerParticles* m_particles = nullptr;

	float m_stateTimer = 0.0f;
	bool m_hasSummoned = false;
	bool m_hasScheduledSummonVfx = false;

	std::vector<Vector3> m_plannedSummonPositions;
};