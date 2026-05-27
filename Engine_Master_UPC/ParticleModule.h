#pragma once

#include "IArchive.h"
#include "JsonArchive.h"

class EmitterInstance;
class Transform;

// Update when needed
enum class ParticleModuleType {

	BASE,
	AREA,
	SPAWN,
	COLOR,
	LIFETIME,
	VELOCITY,
	SIZE
};

class ParticleModule
{

public:

	ParticleModule(ParticleModuleType type) : m_moduleType(type) {}
	virtual std::unique_ptr<ParticleModule> clone() const = 0;

	virtual void spawn(EmitterInstance* particleData) { return; } // Not being used right now...
	virtual void update(EmitterInstance* particleData) { return; }

	ParticleModuleType getType() { return m_moduleType; }

	// Interface and saving/loading functions
	virtual bool drawUi() { return false; }
	virtual void debugDraw(Transform* parent)  {}
	virtual void serialize(IArchive& archive);
	virtual rapidjson::Value getJSON(rapidjson::Document& domTree) {
        JsonArchive archive(ArchiveMode::Output);
        serialize(archive);
        return archive.extractValue(domTree.GetAllocator());
    };
	virtual bool deserializeJSON(const rapidjson::Value& moduleInfo) {
        JsonArchive archive(ArchiveMode::Input);
        archive.setValue(moduleInfo);
        serialize(archive);
        return true;
    }

private:

	const ParticleModuleType m_moduleType;
};

