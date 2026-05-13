#pragma once

#include "ParticleModule.h"

class EmitterSpawn : public ParticleModule
{
public:

	EmitterSpawn() : ParticleModule(ParticleModuleType::SPAWN) {}

	void update(EmitterInstance* particleData) override;

private:

	bool m_looping = true; // if true, ignores m_duration and spawns infinitely
	float m_duration = 20.f;

	float m_rateOverTime = 10.f;
	float m_rateOverDistance = 0.f;
};

