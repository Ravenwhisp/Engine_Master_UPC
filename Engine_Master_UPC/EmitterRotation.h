#pragma once

#include "ParticleModule.h"

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
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

private:

	// We will want Bezier curves to tweak these better (size, speed?)
	// (Rotations in radians)
	float m_startRotation = 0.f;
	
	float m_angularVelocity = 0.f;
	float m_flipRotationLikelihood = 0.f; // how likely is that we rotate in the opposite direction ([0, 1], the bigger the more likely)

	float getNormalizedAngle(float angle);
};

