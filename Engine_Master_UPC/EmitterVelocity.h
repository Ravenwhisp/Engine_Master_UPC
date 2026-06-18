#pragma once

#include "ParticleModule.h"
#include <array>

class EmitterVelocity : public ParticleModule
{
public:

	EmitterVelocity() : ParticleModule(ParticleModuleType::VELOCITY) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterVelocity>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

private:

	ParameterType m_velocityType = ParameterType::CONSTANT;
	float m_initialVelocity = 5.0f;
	float m_initialVelocity2 = 5.0f; // Si no es constante
	float m_velocityCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f }; // Si es CURVE

	bool drawVelocityUI();
};

