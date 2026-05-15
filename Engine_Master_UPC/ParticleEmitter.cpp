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
}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}


rapidjson::Value ParticleEmitter::getJSON(rapidjson::Document& domTree) {

	rapidjson::Value emitterInfo(rapidjson::kObjectType);

	// --- We will probably want to have the textureAssetID here in the future; for now it will be like this

	rapidjson::Value moduleData(rapidjson::kArrayType);
	for (auto& module : m_particleModules)
	{
		moduleData.PushBack(module->getJSON(domTree), domTree.GetAllocator());
	}

	emitterInfo.AddMember("Modules", moduleData, domTree.GetAllocator());

	return emitterInfo;
}
