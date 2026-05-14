#pragma once

#include "ParticleModule.h"

class EmitterColor : public ParticleModule
{
public:

	EmitterColor() : ParticleModule(ParticleModuleType::COLOR) {}

	void update(EmitterInstance* particleData) override;

	void setCreationColor(const Vector4& creationColor) { m_creationColor = creationColor; }
	const Vector4& getCreationColor() const { return m_creationColor; }

	void setStartColor(const Vector4& startColor) { m_startColor = startColor; }
	const Vector4& getStartColor() const { return m_startColor; }

	void setEndColor(const Vector4& endColor) { m_endColor = endColor; }
	const Vector4& getEndColor() const { return m_endColor; }

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:

	Vector4 m_creationColor = Vector4(1.f, 1.f, 1.f, 1.f);

	// We will want Bezier curves to tweak this better (Vector4 size, speed?)
	Vector4 m_startColor = Vector4(1.f, 1.f, 1.f, 1.f);
	Vector4 m_endColor = Vector4(1.f, 1.f, 1.f, 1.f);
};

