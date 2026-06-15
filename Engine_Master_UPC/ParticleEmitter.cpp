#include "Globals.h"
#include "ParticleEmitter.h"

#include "EmitterSpawn.h"
#include "EmitterLifetime.h"
#include "EmitterArea.h"
#include "EmitterColor.h"
#include "EmitterVelocity.h"
#include "EmitterSize.h"
#include "EmitterRotation.h"
#include "EmitterAnimation.h"

ParticleEmitter::ParticleEmitter()
{
	// push_back of all modules we create (IF WE WANT ADDMODULE(TYPE), WE COULD HAVE THEM ALL AS NULL, AND INITIALIZE THEM AS NEEDED)
	// We may be able to make this more automatic if we iterate over the emitterType enum... (maybe not here, but in the other functions)

	// (Spawn here)
	m_particleModules.push_back(std::make_unique<EmitterSpawn>());

	auto emitterLifeTime = std::make_unique<EmitterLifetime>();
	m_lifetimeModule = emitterLifeTime.get();
	m_particleModules.push_back(std::move(emitterLifeTime));

	m_particleModules.push_back(std::make_unique<EmitterArea>());
	m_particleModules.push_back(std::make_unique<EmitterColor>());
	m_particleModules.push_back(std::make_unique<EmitterVelocity>());
	m_particleModules.push_back(std::make_unique<EmitterSize>());
	m_particleModules.push_back(std::make_unique<EmitterRotation>());

	auto emitterAnimation = std::make_unique<EmitterAnimation>();
	m_animationModule = emitterAnimation.get();
	m_particleModules.push_back(std::move(emitterAnimation));
}

ParticleEmitter::ParticleEmitter(const ParticleEmitter& particleEmitter)
{
	m_texture = particleEmitter.m_texture;

	// Particle modules copy //
	m_particleModules.reserve(particleEmitter.m_particleModules.size());

	m_particleModules.push_back(particleEmitter.m_particleModules[0]->clone());

	auto emitterLifetime = particleEmitter.m_particleModules[1]->clone();
	m_lifetimeModule = static_cast<EmitterLifetime*>(emitterLifetime.get());
	m_particleModules.push_back(std::move(emitterLifetime));

	m_particleModules.push_back(particleEmitter.m_particleModules[2]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[3]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[4]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[5]->clone());
	m_particleModules.push_back(particleEmitter.m_particleModules[6]->clone());

	auto emitterAnimation = particleEmitter.m_particleModules[7]->clone();
	m_animationModule = static_cast<EmitterAnimation*>(emitterAnimation.get());
	m_particleModules.push_back(std::move(emitterAnimation));
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

bool ParticleEmitter::deserializeJSON(const rapidjson::Value& emitterInfo) {

	if (!emitterInfo.HasMember("Modules")) return false;

	const rapidjson::Value& modulesInfo = emitterInfo["Modules"];

	for (auto& moduleData : modulesInfo.GetArray()) 
	{
		if (!moduleData.HasMember("ModuleType")) continue;

		unsigned int typeUInt = moduleData["ModuleType"].GetUint();
		ParticleModuleType moduleType = static_cast<ParticleModuleType>(typeUInt);

		switch (moduleType) { // WE SHOULD SERIOUSLY CONSIDER HAVING THE MODULES SEPARATED...

		case ParticleModuleType::BASE:

			// Not implemented yet
			break;

		case ParticleModuleType::AREA:

			m_particleModules[2]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::SPAWN:

			m_particleModules[0]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::COLOR:

			m_particleModules[3]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::LIFETIME:

			m_particleModules[1]->deserializeJSON(moduleData);
			break;
		
		case ParticleModuleType::VELOCITY:

			m_particleModules[4]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::SIZE:

			m_particleModules[5]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::ROTATION:

			m_particleModules[6]->deserializeJSON(moduleData);
			break;

		case ParticleModuleType::ANIMATION:

			m_particleModules[7]->deserializeJSON(moduleData);
		}
	}
	return true;
}