#pragma once

#include "Globals.h"

class ParticleEmitter;
class ParticleSystemComponent;

struct Particle {

	Vector3 position;
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

	EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner) {}

	void init();
	void updateModules();

	ParticleEmitter* getParticleEmitter() { return m_emitter; }
	ParticleSystemComponent* getParticleSystemComponent() { return m_owner; }

	void getPoolAndAlives(Particle*& particlePool, std::vector<std::pair<unsigned int, unsigned int>>*& aliveParticles)
	{ particlePool = m_pool; aliveParticles = &m_aliveParticles; }

	Particle* getParticlePool() { return m_pool; }

	std::vector<unsigned int>& getNewParticles() { return m_newParticles; }

private:

	ParticleEmitter* m_emitter;
	ParticleSystemComponent* m_owner;

	Particle m_pool[300];
	//std::vector<bool> activatedParticles(300, false);

	std::vector<std::pair<unsigned int, unsigned int>> m_aliveParticles; // saves distance to camera + index to pool

	std::vector<unsigned int> m_newParticles; // holds recently created ones, so that we ONLY initialize them on this frame

	void manageNewParticles();
};

