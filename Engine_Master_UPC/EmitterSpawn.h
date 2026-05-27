#pragma once

#include "ParticleModule.h"

class EmitterSpawn : public ParticleModule
{
public:

	EmitterSpawn() : ParticleModule(ParticleModuleType::SPAWN) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterSpawn>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	bool drawUi() override;
	void serialize(IArchive& archive) override;

private:

	bool m_looping = true; // if true, ignores m_duration and spawns infinitely
	float m_duration = 20.f;

	float m_rateOverTime = 10.f;
	float m_rateOverDistance = 0.f;
};

