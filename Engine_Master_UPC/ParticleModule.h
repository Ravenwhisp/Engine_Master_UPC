#pragma once

class ParticleEmitter;

// Update when needed
enum class ParticleModuleType {

	BASE,
	SPAWN,
	COLOR,
	LIFETIME
};

class ParticleModule
{

public:

	ParticleModule(ParticleModuleType type) : m_moduleType(type) {}

	virtual void spawn(ParticleEmitter*) { return; }
	virtual void update(ParticleEmitter*) { return; }

	ParticleModuleType getType() { return m_moduleType; }

private:

	ParticleModuleType m_moduleType;
};

