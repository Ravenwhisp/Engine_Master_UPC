#pragma once

#include "ParticleModule.h"

class Particle;

class EmitterRotation : public ParticleModule
{
public:

	EmitterRotation() : ParticleModule(ParticleModuleType::ROTATION) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterRotation>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	void setStartRotation(float startRotation) { m_startRotation = startRotation; }
	float getStartRotation() const { return m_startRotation; }

	void setAngularVelocity(float angularVelocity) { m_angularVelocity = angularVelocity; }
	float getAngularVelocity() const { return m_angularVelocity; }

	void setFlipRotationLikelihood(float flipRotation) { m_flipRotationLikelihood = flipRotation; }
	float getFlipRotationLikelihood() const { return m_flipRotationLikelihood; }

	bool drawUi() override;
	void serialize(IArchive& archive) override;

private:

	// We will want Bezier curves to tweak these better (size, speed?)
	// (Rotations in radians)
	float m_startRotation = 0.f;
	

	ParameterType m_angularVelocityType = ParameterType::CONSTANT;
	float m_angularVelocity = 0.f;
	float m_angularVelocity2 = 0.f; // if type != constant
	float m_angularVelocityCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f }; // if type = CURVE

	float m_flipRotationLikelihood = 0.f; // how likely is that we rotate in the opposite direction ([0, 1], the bigger the more likely)

	bool drawAngularVelocityUI();

	void updateAlivesRotationFixed(Particle* particlePool, const std::vector<std::pair<float, unsigned int>>* aliveParticles, float deltaTime);
	void updateAlivesRotationWithCurve(Particle* particlePool, const std::vector<std::pair<float, unsigned int>>* aliveParticles, float deltaTime, float startLifeTime);

	void setNewParticlesRotationFixed(Particle* particlePool, const std::vector<unsigned int>& newParticles);
	void setNewParticlesRotationWithRange(Particle* particlePool, const std::vector<unsigned int>& newParticles);


	float getNormalizedAngle(float angle);
};

