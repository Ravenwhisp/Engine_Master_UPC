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
	Vector3 getHDRColorAndIntensity() const { return m_hdrColorAndIntensity; }

	bool drawUi() override;
	void serialize(IArchive& archive) override;
	
private:

	// For now, we will have gradient + Bezier curve (we may want more options, like in Unity)

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	float m_colorCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f };

	// for bloom post-processing, we will have a HDR color + intensity
	float m_hdrColor[3] = { 0.000f, 0.000f, 0.000f};
	float m_intensity = 0.f;

	Vector3 m_hdrColorAndIntensity = Vector3(0.f, 0.f, 0.f); //  final value, to be used in the shader


	bool drawBezierCurveUI(float* curveData);
	bool drawHDRColorUI(float* colorData, float* intensity, Vector3* hdrEndColor);
	Vector3 getFinalHDRColor(float* colorData, float* intensity) const;


};

