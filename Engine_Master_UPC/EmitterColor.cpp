#include "Globals.h"
#include "EmitterColor.h"

#include "Application.h"
#include "ModuleInput.h"
#include "imgui_bezier.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

EmitterColor::EmitterColor() : ParticleModule(ParticleModuleType::COLOR) {

	
	m_colorOverTime.getMarks().clear(); // because it has default values that we don't want

	m_colorOverTime.addMark(0.f, ImColor(1.f, 1.f, 1.f, 1.f));
	m_colorOverTime.addMark(1.f, ImColor(1.f, 1.f, 1.f, 1.f));
}


void EmitterColor::update(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();

		// Dealing with already existing particles //

		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float scale = 1.f - particlePool[poolIndex].lifeTime / startLifetime; // to start with 0
			float bezierScale = ImGui::BezierValue(scale, m_colorCurve);

			m_colorOverTime.getColorAt(bezierScale, &particlePool[poolIndex].colorAndAlpha.x);
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		m_colorOverTime.getColorAt(0.f, &particlePool[particleIndex].colorAndAlpha.x);
	}
}

bool EmitterColor::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Color")) 
	{	
		// Color gradient editor
		if (ImGui::GradientButton(&m_colorOverTime)) {

			ImGui::OpenPopup("GradientEditorPopup");
		}

		ImGui::SameLine(); ImGui::Text("Color over time");


		if (ImGui::BeginPopup("GradientEditorPopup")) {
			
			//ImGui::PushID("GradientID"); // to avoid conflicting index error
			//size_t beforeMarkers = m_colorsOverTime.getMarks().size();

			parameterChanged |= ImGui::GradientEditor(&m_colorOverTime, m_draggingMark, m_selectedMark);

			if (app->getModuleInput()->isKeyJustPressed(Keyboard::Keys::Delete) && m_selectedMark != nullptr)
			{
				m_colorOverTime.removeMark(m_selectedMark);
			}

			//ImGui::PopID(); // (same, corresponding)

			ImGui::EndPopup();
		}


		parameterChanged |= drawBezierCurveUI(m_colorCurve);
	}

	return parameterChanged;
}

void EmitterColor::serialize(IArchive& archive)
{
	ParticleModule::serialize(archive);

	if (archive.mode() == ArchiveMode::Output)
	{
		const auto& marks = m_colorOverTime.getMarks();
		uint32_t markCount = static_cast<uint32_t>(marks.size());
		archive.beginArray(markCount, "ColorOverTime");

		for (const ImGradientMark* mark : marks)
		{
			archive.beginObject();
			bool isAlphaMark = mark->alpha;
			float position = mark->position;
			archive.serialize(isAlphaMark, "IsAlphaMark");
			archive.serialize(position, "Position");

			if (mark->alpha)
			{
				float alpha = mark->color[0];
				archive.serialize(alpha, "Alpha");
			}
			else
			{
				uint32_t colorCount = 3;
				archive.beginArray(colorCount, "Color");
				float r = mark->color[0];
				float g = mark->color[1];
				float b = mark->color[2];
				archive.serialize(r, "");
				archive.serialize(g, "");
				archive.serialize(b, "");
				archive.endArray();
			}

			archive.endObject();
		}

		archive.endArray();
	}
	else
	{
		uint32_t markCount = 0;
		archive.beginArray(markCount, "ColorOverTime");
		m_colorOverTime.clearMarks();

		for (uint32_t i = 0; i < markCount; ++i)
		{
			archive.beginObject();

			bool isAlphaMark = false;
			float position = 0.f;
			archive.serialize(isAlphaMark, "IsAlphaMark");
			archive.serialize(position, "Position");

			if (isAlphaMark)
			{
				float alpha = 0.f;
				archive.serialize(alpha, "Alpha");
				m_colorOverTime.addAlphaMark(position, alpha);
			}
			else
			{
				uint32_t colorCount = 3;
				archive.beginArray(colorCount, "Color");
				float r = 0.f, g = 0.f, b = 0.f;
				archive.serialize(r, "");
				archive.serialize(g, "");
				archive.serialize(b, "");
				archive.endArray();
				m_colorOverTime.addMark(position, ImColor(r, g, b));
			}

			archive.endObject();
		}

		archive.endArray();
	}

	{
		uint32_t curveCount = 4;
		archive.beginArray(curveCount, "ColorCurve");
		for (int i = 0; i < 4; ++i)
			archive.serialize(m_colorCurve[i], "");
		archive.endArray();
	}
}

bool EmitterColor::drawBezierCurveUI(float* curveData)
{
	bool parameterChanged = false;

	// (Values between 0 and 1)
	if (ImGui::Bezier("Curve##Color", curveData))
	{
		parameterChanged = true;
	}

	// We add some buttons to quickly change to predefined setups
	if (ImGui::Button("Linear##Color"))
	{
		curveData[0] = 0.000f; curveData[1] = 0.000f; curveData[2] = 1.000f; curveData[3] = 1.000f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseIn##Color"))
	{
		curveData[0] = 0.470f; curveData[1] = 0.000f; curveData[2] = 0.745f; curveData[3] = 0.715f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseOut##Color"))
	{
		curveData[0] = 0.390f; curveData[1] = 0.575f; curveData[2] = 0.565f; curveData[3] = 1.000f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseInOut##Color"))
	{
		curveData[0] = 0.445f; curveData[1] = 0.050f; curveData[2] = 0.550f; curveData[3] = 0.950f;
		parameterChanged = true;
	}

	return parameterChanged;
}
