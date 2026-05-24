#pragma once

#include "ParticleEmitter.h"

class ParticleSystem
{
public:

	ParticleSystem(unsigned int numEmitters = 0);
	ParticleSystem(const ParticleSystem& particleSystem);

	std::vector<ParticleEmitter>& getEmitters() { return m_emitters; }
	ParticleEmitter* addEmitter() { m_emitters.push_back(ParticleEmitter()); return &m_emitters.back(); }

private:

	std::vector<ParticleEmitter> m_emitters;
};

