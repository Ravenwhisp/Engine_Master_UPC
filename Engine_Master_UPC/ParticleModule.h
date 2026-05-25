#pragma once

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
	virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization
	virtual bool deserializeJSON(const rapidjson::Value& moduleInfo) { return true; }

private:

	const ParticleModuleType m_moduleType;
};

