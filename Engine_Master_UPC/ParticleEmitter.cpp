#include "Globals.h"
#include "ParticleEmitter.h"
#include "JsonArchive.h"

#include "EmitterSpawn.h"
#include "EmitterLifetime.h"
#include "EmitterArea.h"
#include "EmitterColor.h"
#include "EmitterVelocity.h"
#include "EmitterSize.h"
#include "EmitterRotation.h"

ParticleEmitter::ParticleEmitter()
{
	// push_back of all modules we create (IF WE WANT ADDMODULE(TYPE), WE COULD HAVE THEM ALL AS NULL, AND INITIALIZE THEM AS NEEDED)
	// We may be able to make this more automatic if we iterate over the emitterType enum... (maybe not here, but in the other functions)

	// (Spawn here)
	m_particleModules.push_back(std::make_unique<EmitterSpawn>());

	auto emitterLifeTime = std::make_unique<EmitterLifetime>();
	m_lifeTimeModule = emitterLifeTime.get();
	m_particleModules.push_back(std::move(emitterLifeTime));



	m_particleModules.push_back(std::make_unique<EmitterArea>());
	m_particleModules.push_back(std::make_unique<EmitterColor>());
	m_particleModules.push_back(std::make_unique<EmitterVelocity>());
	m_particleModules.push_back(std::make_unique<EmitterSize>());
	m_particleModules.push_back(std::make_unique<EmitterRotation>());
}

ParticleEmitter::ParticleEmitter(const ParticleEmitter& particleEmitter)
{
	m_texture = particleEmitter.m_texture;

	// Particle modules copy //
	m_particleModules.reserve(particleEmitter.m_particleModules.size());

	m_particleModules.push_back(particleEmitter.m_particleModules[0]->clone());

	auto emitterLifeTime = particleEmitter.m_particleModules[1]->clone();
	m_lifeTimeModule = static_cast<EmitterLifetime*>(emitterLifeTime.get());
	m_particleModules.push_back(std::move(emitterLifeTime));

	m_particleModules.push_back(particleEmitter.m_particleModules[2]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[3]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[4]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[5]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[6]->clone());
}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}


void ParticleEmitter::serialize(IArchive& archive)
{
    uint32_t moduleCount = static_cast<uint32_t>(m_particleModules.size());
    archive.serialize(moduleCount, "ModuleCount");

    if (moduleCount > m_particleModules.size())
        moduleCount = static_cast<uint32_t>(m_particleModules.size());

    for (uint32_t i = 0; i < moduleCount; ++i)
    {
        std::string key = "Module_" + std::to_string(i);
        archive.beginObject(key.c_str());
        m_particleModules[i]->serialize(archive);
        archive.endObject();
    }
}



