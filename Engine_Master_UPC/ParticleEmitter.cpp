#include "Globals.h"
#include "ParticleEmitter.h"

#include "EmitterSpawn.h"
#include "EmitterLifetime.h"
#include "EmitterArea.h"
#include "EmitterColor.h"
#include "EmitterVelocity.h"

ParticleEmitter::ParticleEmitter()
{
	// push_back of all modules we create (IF WE WANT ADDMODULE(TYPE), WE COULD HAVE THEM ALL AS NULL, AND INITIALIZE THEM AS NEEDED)
	
	// (Spawn here)
	m_particleModules.push_back(std::make_unique<EmitterSpawn>());

	auto emitterLifeTime = std::make_unique<EmitterLifetime>();
	m_lifeTimeModule = emitterLifeTime.get();
	m_particleModules.push_back(std::move(emitterLifeTime));



	m_particleModules.push_back(std::make_unique<EmitterArea>());
	m_particleModules.push_back(std::make_unique<EmitterColor>());
	m_particleModules.push_back(std::make_unique<EmitterVelocity>());
}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}
