#include "Globals.h"
#include "ParticleEmitter.h"

#include "EmitterLifetime.h"

ParticleEmitter::ParticleEmitter()
{
	// push_back of all modules we create (IF WE WANT ADDMODULE(TYPE), WE COULD HAVE THEM ALL AS NULL, AND INITIALIZE THEM AS NEEDED)
	
	// (Spawn here)

	auto emitterLifeTime = std::make_unique<EmitterLifetime>();
	m_lifeTimeModule = emitterLifeTime.get();
	m_particleModules.push_back(std::move(emitterLifeTime));



}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}
