#pragma once

#include "ParticleModule.h"

class EmitterVelocity : public ParticleModule
{
public:

	EmitterVelocity() : ParticleModule(ParticleModuleType::VELOCITY) {}

	void update(EmitterInstance* particleData) override;

private:

	float m_initialVelocity = 5.0f;

};

