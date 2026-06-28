#pragma once

#include "ParticleModule.h"
#include "ModuleParticleSystem.h"

class EmitterAnimation : public ParticleModule
{
public:

	EmitterAnimation() : ParticleModule(ParticleModuleType::ANIMATION) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterAnimation>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	int getSheetRows() const { return static_cast<int>(m_rows); }
	int getSheetColumns() const { return static_cast<int>(m_columns); }
	Vector2 getUVScale() const;
	Vector2 getUVOffset (int particleIndex) const;

	bool drawUi() override;
	void serialize(IArchive& archive) override;

private:

	uint32_t m_rows = 1;
	uint32_t m_columns = 1;

	float m_fps = 0.f;

	ParameterType m_startFrameType = ParameterType::CONSTANT;
	float m_startFrame = 0.f; // (value in range [0, 1), to calculate actual frame)
	float m_startFrame2 = 0.f; // if m_startFrameType != CONSTANT

	float m_totalFrames = 1.f; // actually rows * columns (will exist to make calculations faster)

	bool drawStartFrameUI();

	void setNewParticlesFrameConstant(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles);
	void setNewParticlesFrameRandom(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles);
};

