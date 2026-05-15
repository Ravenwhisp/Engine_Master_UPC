#include "Globals.h"
#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(unsigned int numEmitters)
{
	m_emitters.resize(numEmitters);	
}

ParticleSystem::ParticleSystem(const ParticleSystem& particleSystem)
{
	m_emitters.reserve(particleSystem.m_emitters.size());

	for (auto& emitter : particleSystem.m_emitters) 
	{
		m_emitters.push_back(ParticleEmitter(emitter));
	}
}
