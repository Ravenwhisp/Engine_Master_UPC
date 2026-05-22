#pragma once

#include "ParticleModule.h"

class EmitterSize : public ParticleModule
{
public:

	EmitterSize() : ParticleModule(ParticleModuleType::SIZE) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterSize>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	void setStartScale(const Vector2& startScale) { m_startScale = startScale; }
	const Vector2& getStartScale() const { return m_startScale; }

	void setEndScale(const Vector2& endScale) { m_endScale = endScale; }
	const Vector2& getEndScale() const { return m_endScale; }

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

private:

	// We will want Bezier curves to tweak this better (Vector4 size, speed?)
	Vector2 m_startScale = Vector2(1.f, 1.f);
	Vector2 m_endScale = Vector2(1.f, 1.f);
};

