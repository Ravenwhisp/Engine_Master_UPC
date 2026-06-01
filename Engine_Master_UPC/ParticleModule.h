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
	SIZE,
	ROTATION
};

enum class ParameterType {

	CONSTANT,
	RANDOM_BETWEEN_TWO,
	CURVE,
	TOTAL_TYPES
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

private:

	const ParticleModuleType m_moduleType;
};

