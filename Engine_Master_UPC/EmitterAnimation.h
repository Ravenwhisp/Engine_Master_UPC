#pragma once

#include "ParticleModule.h"

class EmitterAnimation : public ParticleModule
{
public:

	EmitterAnimation() : ParticleModule(ParticleModuleType::ANIMATION) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterAnimation>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	int getSheetRows() const { return m_rows; }
	int getSheetColumns() const { return m_columns; }
	Vector2 getUVScale() const;
	Vector2 getUVOffset (int particleIndex) const;

	bool drawUi() override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

private:

	int m_rows = 1;
	int m_columns = 1;

	float m_fps = 0.f;

	float m_totalFrames = 1.f; // actually rows * columns (will exist to make calculations faster)
};

