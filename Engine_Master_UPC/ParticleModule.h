#pragma once

class EmitterInstance;

// Update when needed
enum class ParticleModuleType {

	BASE,
	SPAWN,
	COLOR,
	LIFETIME,
	VELOCITY
};

class ParticleModule
{

public:

	ParticleModule(ParticleModuleType type) : m_moduleType(type) {}

	virtual void spawn(EmitterInstance* particleData) { return; }
	virtual void update(EmitterInstance* particleData) { return; }

	ParticleModuleType getType() { return m_moduleType; }

private:

	const ParticleModuleType m_moduleType;
};

