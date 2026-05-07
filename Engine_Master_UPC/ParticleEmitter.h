#pragma once

#include "ParticleModule.h"
#include "Transform.h"
#include "Texture.h"
#include <vector>
#include <utility>

class ParticleEmitter
{
public:

	ParticleEmitter();

	ParticleModule* getModule(ParticleModuleType type);
	std::vector<std::unique_ptr<ParticleModule>>& getModules() { return m_particleModules; }

private:

	Texture* m_texture;

	std::vector<std::unique_ptr<ParticleModule>> m_particleModules;
};

