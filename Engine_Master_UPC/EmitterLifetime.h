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
	void serialize(IArchive& archive) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

private:
	ParameterType m_lifeTimeType = ParameterType::CONSTANT;
	float m_startLifeTime = 5.0f;
	float m_startLifeTime2 = 5.0f;

	void eraseBySwap(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index); // swaps the element at position = index with the back and pops it (does not respect order, but should be faster)
	void swapWithBack(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index);
};

