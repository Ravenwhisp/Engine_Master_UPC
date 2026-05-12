#pragma once

#include "ParticleModule.h"

class EmitterSpawn : public ParticleModule
{
public:

	EmitterSpawn() : ParticleModule(ParticleModuleType::SPAWN) {}

	void update(EmitterInstance* particleData) override;

private:

	float m_rateOverTime = 10.f;
	float m_rateOverDistance = 0.f;
};

