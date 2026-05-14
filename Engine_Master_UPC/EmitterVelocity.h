#pragma once

#include "ParticleModule.h"

class EmitterVelocity : public ParticleModule
{
public:

	EmitterVelocity() : ParticleModule(ParticleModuleType::VELOCITY) {}

	void update(EmitterInstance* particleData) override;

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:

	float m_initialVelocity = 5.0f;
};

