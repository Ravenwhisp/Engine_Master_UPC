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
#include "EmitterAnimation.h"
#include "EmitterRender.h"

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

	auto emitterRender = std::make_unique<EmitterRender>();
	m_renderModule = emitterRender.get();
	m_particleModules.push_back(std::move(emitterRender));
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

	auto emitterRender = particleEmitter.m_particleModules[8]->clone();
	m_renderModule = static_cast<EmitterRender*>(emitterRender.get());
	m_particleModules.push_back(std::move(emitterRender));
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
	if (archive.mode() == ArchiveMode::Output)
	{
		uint32_t moduleCount = static_cast<uint32_t>(m_particleModules.size());
		archive.beginArray(moduleCount, "Modules");

		for (uint32_t i = 0; i < moduleCount; ++i)
		{
			archive.beginObject();
			m_particleModules[i]->serialize(archive);
			archive.endObject();
		}

		archive.endArray();
	}
	else // Input: cargar con deserializeJSON
	{
		JsonArchive* jsonArchive = dynamic_cast<JsonArchive*>(&archive);
		if (!jsonArchive)
			return;

		const rapidjson::Value* emitterInfo = jsonArchive->currentInput();
		if (emitterInfo)
			deserializeJSON(*emitterInfo);
	}
}

bool ParticleEmitter::deserializeJSON(const rapidjson::Value& emitterInfo)
{
	if (!emitterInfo.HasMember("Modules"))
		return false;

	const rapidjson::Value& modulesInfo = emitterInfo["Modules"];

	for (auto& moduleData : modulesInfo.GetArray())
	{
		if (!moduleData.HasMember("ModuleType")) continue;

		unsigned int typeUInt = moduleData["ModuleType"].GetUint();
		ParticleModuleType moduleType = static_cast<ParticleModuleType>(typeUInt);

		ParticleModule* module = getModule(moduleType);
		if (!module)
			continue;

		module->deserializeJSON(moduleData);
	}

	return true;
}