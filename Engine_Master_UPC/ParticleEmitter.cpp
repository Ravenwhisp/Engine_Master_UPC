#include "Globals.h"
#include "ParticleEmitter.h"

ParticleEmitter::ParticleEmitter(Transform* parentTransform) : m_parent(parentTransform)
{
	// push_back of all modules we create
}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}
