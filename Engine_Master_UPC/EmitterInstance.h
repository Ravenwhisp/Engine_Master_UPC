#pragma once

#include "Globals.h"

class ParticleEmitter;
class ParticleSystemComponent;

struct Particle {

	Vector3 position; // CHANGE TO A TRANSFORM?
	Vector4 colorAndAlpha;
	float rotationZ; // since they are billboards, other rotations don't make sense

	float velocity;
	Vector3 movementDirection; // should be normalized

	float lifeTime = 0.f;
	bool alive = false; // could be replaced by vector<bool> inside Emitter (also: depending if we check lifeTime earlier or later, we might get away without this)
};

class EmitterInstance
{
public:

	EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner);

	void init();
	void updateModules();

	ParticleEmitter* getParticleEmitter() { return m_emitter; }
	ParticleSystemComponent* getParticleSystemComponent() { return m_owner; }

	void getPoolAndAlives(Particle*& particlePool, std::vector<std::pair<unsigned int, unsigned int>>*& aliveParticles)
	{ particlePool = m_pool; aliveParticles = &m_aliveParticles; }

	Particle* getParticlePool() { return m_pool; }

	std::vector<unsigned int>& getNewParticles() { return m_newParticles; }
	float getParticlesToSpawn() { return m_particlesToSpawn; }
	void setParticlesToSpawn(float particles) { m_particlesToSpawn = particles; }

	// Slot management //
	int requestPoolSlot(); // returns a free pool slot, -1 if none available

	void freePoolSlot(unsigned int index); // frees the slot at the index (BUT DOES NOT CHANGE THE OTHER ARRAYS!) - TO CHANGE?

private:

	ParticleEmitter* m_emitter;
	ParticleSystemComponent* m_owner;

	Particle m_pool[MAX_PARTICLES];

	// Slot management data
	unsigned int m_slots[MAX_PARTICLES]; // contains for each particle the next free (or self if nothing free)
	unsigned int m_firstFree = 0;
	//std::vector<bool> activatedParticles(300, false);

	std::vector<std::pair<float, unsigned int>> m_aliveParticles; // saves distance (squared, because it is easier to calculate) to camera + index to pool

	std::vector<unsigned int> m_newParticles; // holds recently created ones, so that we ONLY initialize them on this frame (so contains particles generated on the current frame)
	float m_particlesToSpawn = 0.f;

	//std::vector<unsigned int> m_alivesperLifetimeOrder; <- TO DO, for proper spawning

	void manageNewParticles();
};

