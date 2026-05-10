#pragma once

#include "ParticleModule.h"

class EmitterInstance;

class EmitterLifetime : public ParticleModule
{
public:

	EmitterLifetime() : ParticleModule(ParticleModuleType::LIFETIME) {}

	void update(EmitterInstance* particleData) override;

	void setStartLifetime(float startLifetime) { m_startLifeTime = startLifetime; }
	float getStartLifetime() { return m_startLifeTime; }

private:

	float m_startLifeTime = 5.0f;
};

