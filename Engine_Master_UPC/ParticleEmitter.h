#pragma once

#include "ParticleModule.h"
#include "Transform.h"
#include "Texture.h"
#include <vector>
#include <utility>

class EmitterLifetime;

class ParticleEmitter
{
public:

	ParticleEmitter();

	ParticleModule* getModule(ParticleModuleType type);
	std::vector<std::unique_ptr<ParticleModule>>& getModules() { return m_particleModules; }

	void setTexture(Texture* texture) { m_texture = texture; }
	Texture* getTexture() { return m_texture; }

	EmitterLifetime* getLifetimeModule() { return m_lifeTimeModule; }


private:

	Texture* m_texture = nullptr;
	
	std::vector<std::unique_ptr<ParticleModule>> m_particleModules;
	EmitterLifetime* m_lifeTimeModule;

	// Main values (that modules will take into account) // <- We will probably not do this, to simplify the format
	bool m_loops = true;
	float m_systemDuration = 5.0f; // only considered if m_loops = false

	// float m_startLifeTime = 5.0f;
	float m_startSpeed = 5;
	Vector4 m_startColor = Vector4 (1.0f, 1.0f, 1.0f, 1.f);
};

