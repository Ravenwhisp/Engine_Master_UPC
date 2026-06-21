#pragma once

#include "ParticleModule.h"

#include "imgui_color_gradient.h"

class EmitterColor : public ParticleModule
{
public:

	EmitterColor();
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterColor>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	// This will return if needed
	//void setStartColor(const Vector4& startColor) { m_startColor = startColor; }
	//const Vector4& getStartColor() const { return m_startColor; }

	ImGradient& getColorGradient() { return m_colorOverTime; }

	bool drawUi() override;
	void serialize(IArchive& archive) override;
	bool deserializeJSON(const rapidjson::Value& moduleInfo) override;
	
private:

	// For now, we will have gradient + Bezier curve (we may want more options, like in Unity)

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	float m_colorCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f };

	bool drawBezierCurveUI(float* curveData);
};

