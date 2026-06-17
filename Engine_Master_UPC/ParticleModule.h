#pragma once

class EmitterInstance;
class Transform;

// Update when needed
enum class ParticleRenderMode {
	BILLBOARD = 0,
	HORIZONTAL = 1,
	VERTICAL = 2
};

enum class ParticleModuleType {

	BASE,
	AREA,
	SPAWN,
	COLOR,
	LIFETIME,
	VELOCITY,
	SIZE,
	ROTATION,
	ANIMATION,
	RENDER
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
	virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization
	virtual bool deserializeJSON(const rapidjson::Value& moduleInfo) { return true; }

private:

	const ParticleModuleType m_moduleType;
};

class EmitterRender : public ParticleModule
{
public:

	enum class RenderMode {
		BILLBOARD = 0,
		HORIZONTAL = 1,
		VERTICAL = 2
	};

	EmitterRender() : ParticleModule(ParticleModuleType::RENDER) {}
	std::unique_ptr<ParticleModule> clone() const override { return std::make_unique<EmitterRender>(*this); }

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

	RenderMode getRenderMode() const { return m_renderMode; }

private:
	RenderMode m_renderMode = RenderMode::BILLBOARD;
};


