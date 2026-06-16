#pragma once

#include "ParticleModule.h"

class EmitterVelocity : public ParticleModule
{
public:

	EmitterVelocity() : ParticleModule(ParticleModuleType::VELOCITY) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterVelocity>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	bool drawUi() override;
	void serialize(IArchive& archive) override;

private:

	float m_initialVelocity = 5.0f;
};

