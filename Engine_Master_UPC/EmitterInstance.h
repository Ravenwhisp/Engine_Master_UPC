#pragma once

#include "Globals.h"

class ParticleEmitter;
class ParticleSystemComponent;

class EmitterInstance
{
public:

	EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner);
	~EmitterInstance();

	void init();
	void updateModules();

	void reset(); // restart particle instantiation from the beginning

	ParticleEmitter* getParticleEmitter() { return m_emitter; }
	ParticleSystemComponent* getParticleSystemComponent() { return m_owner; }

	std::vector<std::pair<float, unsigned int>>& getAliveParticles() { return m_aliveParticles; }
	std::vector<unsigned int>& getNewParticles() { return m_newParticles; }

	float getParticlesToSpawn() const { return m_particlesToSpawn; }
	void setParticlesToSpawn(float particles) { m_particlesToSpawn = particles; }

	float getCurrentTime() const { return m_currentTime; }

private:

	ParticleEmitter* m_emitter;
	ParticleSystemComponent* m_owner;

	std::vector<std::pair<float, unsigned int>> m_aliveParticles; // saves distance (squared, because it is easier to calculate) to camera + index to pool

	std::vector<unsigned int> m_newParticles; // holds recently created ones, so that the ONLY thing we do this frame is initializing them (contains particles generated on the current frame)
	float m_particlesToSpawn = 0.f;

	//std::vector<unsigned int> m_alivesPerLifetimeOrder; <- TO DO, for proper spawning

	float m_currentTime = 0.f;

	void freeParticleSlots();
	void manageNewParticles();
};

