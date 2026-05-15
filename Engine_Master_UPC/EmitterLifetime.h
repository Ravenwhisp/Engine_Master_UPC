#pragma once

#include "ParticleModule.h"

class EmitterInstance;

class EmitterLifetime : public ParticleModule
{
public:

	EmitterLifetime() : ParticleModule(ParticleModuleType::LIFETIME) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterLifetime>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	void setStartLifetime(float startLifetime) { m_startLifeTime = startLifetime; }
	float getStartLifetime() const { return m_startLifeTime; }

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:

	float m_startLifeTime = 5.0f;
};

