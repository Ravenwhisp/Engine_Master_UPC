#pragma once

#include "ParticleModule.h"
#include "Transform.h"
#include <vector>
#include <utility>


struct Particle {

	Vector3 position;
	Vector4 colorAndAlpha;
	float rotationZ; // since they are billboards, other rotations don't make sense

	float lifeTime = 0.f;
	bool alive = false; // could be replaced by vector<bool> inside Emitter (also: depending if we check lifeTime earlier or later, we might get away without this)
};

class ParticleEmitter
{
public:

	ParticleEmitter(Transform* parentTransform);

	ParticleModule* getModule(ParticleModuleType type);
	std::vector<std::unique_ptr<ParticleModule>>& getModules() { return m_particleModules; }

	void update();

private:

	std::vector<std::unique_ptr<ParticleModule>> m_particleModules;
	Transform* m_parent;

	Particle m_pool[300];
	//std::vector<bool> activatedParticles(300, false);

	std::vector<std::pair<unsigned int, unsigned int>> m_aliveParticles; // saves distance to camera + index to pool
};

