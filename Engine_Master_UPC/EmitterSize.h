#pragma once

#include "ParticleModule.h"
#include "ModuleParticleSystem.h"

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
	void serialize(IArchive& archive) override;

private:

	ParameterType m_startScaleType = ParameterType::CONSTANT;
	Vector2 m_startScale = Vector2(1.f, 1.f);
	Vector2 m_startScale2 = Vector2(1.f, 1.f); // if type != CONSTANT
	float m_startScaleCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f }; // if type = CURVE
	
	bool drawStartScaleUI();
	

	bool m_changeSizeOverTime = false;

	// We will want Bezier curves to tweak this better
	Vector2 m_endScale = Vector2(1.f, 1.f);

	void setNewParticlesScaleConstant(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles);
	void setNewParticlesScaleRandom(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles);
};

