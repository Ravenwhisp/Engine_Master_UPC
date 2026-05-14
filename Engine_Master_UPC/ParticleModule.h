#pragma once

class EmitterInstance;

// Update when needed
enum class ParticleModuleType {

	BASE,
	AREA,
	SPAWN,
	COLOR,
	LIFETIME,
	VELOCITY
};

class ParticleModule
{

public:

	ParticleModule(ParticleModuleType type) : m_moduleType(type) {}

	virtual void spawn(EmitterInstance* particleData) { return; }
	virtual void update(EmitterInstance* particleData) { return; }

	ParticleModuleType getType() { return m_moduleType; }

	// Interface and saving/loading functions
	virtual bool drawUi() { return false; }
	virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization
	virtual bool deserializeJSON(const rapidjson::Value& componentValue) { return true; }

private:

	const ParticleModuleType m_moduleType;
};

