#include "Globals.h"
#include "ParticleEmitter.h"
#include "JsonArchive.h"

#include "EmitterSpawn.h"
#include "EmitterLifetime.h"
#include "EmitterArea.h"
#include "EmitterColor.h"
#include "EmitterVelocity.h"
#include "EmitterSize.h"

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
	m_particleModules.push_back(std::make_unique<EmitterSize>());
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
}

ParticleModule* ParticleEmitter::getModule(ParticleModuleType type)
{
	for (auto& module : m_particleModules) 
	{
		if (module->getType() == type) return module.get();
	}

	return nullptr;
}


rapidjson::Value ParticleEmitter::getJSON(rapidjson::Document& domTree)
{
    JsonArchive archive(ArchiveMode::Output);
    serialize(archive);
    return archive.extractValue(domTree.GetAllocator());
}

void ParticleEmitter::serialize(IArchive& archive)
{
    uint32_t moduleCount = static_cast<uint32_t>(m_particleModules.size());
    archive.serialize(moduleCount, "ModuleCount");

    for (uint32_t i = 0; i < moduleCount; ++i)
    {
        std::string key = "Module_" + std::to_string(i);
        archive.beginObject(key.c_str());
        m_particleModules[i]->serialize(archive);
        archive.endObject();
    }
}

bool ParticleEmitter::deserializeJSON(const rapidjson::Value& emitterInfo)
{
    JsonArchive archive(ArchiveMode::Input);
    archive.setValue(emitterInfo);
    serialize(archive);

    if (emitterInfo.HasMember("Modules"))
    {
        const rapidjson::Value& modulesInfo = emitterInfo["Modules"];
        for (auto& moduleData : modulesInfo.GetArray())
        {
            if (!moduleData.HasMember("ModuleType")) continue;
            unsigned int typeUInt = moduleData["ModuleType"].GetUint();
            ParticleModuleType moduleType = static_cast<ParticleModuleType>(typeUInt);

            switch (moduleType) {
            case ParticleModuleType::AREA:       m_particleModules[2]->deserializeJSON(moduleData); break;
            case ParticleModuleType::SPAWN:      m_particleModules[0]->deserializeJSON(moduleData); break;
            case ParticleModuleType::COLOR:      m_particleModules[3]->deserializeJSON(moduleData); break;
            case ParticleModuleType::LIFETIME:   m_particleModules[1]->deserializeJSON(moduleData); break;
            case ParticleModuleType::VELOCITY:   m_particleModules[4]->deserializeJSON(moduleData); break;
            case ParticleModuleType::SIZE:       m_particleModules[5]->deserializeJSON(moduleData); break;
            default: break;
            }
        }
    }

    return true;
}

