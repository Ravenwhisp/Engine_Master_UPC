#pragma once

#include "ParticleEmitter.h"

class ParticleSystem
{
public:

	ParticleSystem();

	std::vector<ParticleEmitter>& getEmitters() { return m_emitters; }

private:

	std::vector<ParticleEmitter> m_emitters;
};

