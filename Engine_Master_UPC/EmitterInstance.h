#pragma once

class Emitter;
class ParticleSystemComponent;

struct Particle {

	Vector3 position;
	Vector4 colorAndAlpha;
	float rotationZ; // since they are billboards, other rotations don't make sense

	float lifeTime = 0.f;
	bool alive = false; // could be replaced by vector<bool> inside Emitter (also: depending if we check lifeTime earlier or later, we might get away without this)
};

class EmitterInstance
{
public:

	EmitterInstance(Emitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner) {}

	void init();
	void updateModules();

private:

	Emitter* m_emitter;
	ParticleSystemComponent* m_owner;

	Particle m_pool[300];
	//std::vector<bool> activatedParticles(300, false);

	std::vector<std::pair<unsigned int, unsigned int>> m_aliveParticles; // saves distance to camera + index to pool
};

