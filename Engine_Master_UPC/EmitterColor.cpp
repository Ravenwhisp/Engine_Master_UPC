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

rapidjson::Value EmitterColor::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::COLOR), domTree.GetAllocator());

	// Get color gradient data (marks)

	rapidjson::Value gradientData(rapidjson::kArrayType);

	for (const auto& mark : m_colorOverTime.getMarks()) 
	{
		rapidjson::Value markData(rapidjson::kObjectType);

		// Mark parameters //

		markData.AddMember("IsAlphaMark", mark->alpha, domTree.GetAllocator());

		if (mark->alpha) 
		{
			markData.AddMember("Alpha", mark->color[0], domTree.GetAllocator());
		}
		else // => is a color marker 
		{
			rapidjson::Value colorData(rapidjson::kArrayType);

			colorData.PushBack(mark->color[0], domTree.GetAllocator());
			colorData.PushBack(mark->color[1], domTree.GetAllocator());
			colorData.PushBack(mark->color[2], domTree.GetAllocator());

			markData.AddMember("Color", colorData, domTree.GetAllocator());
		}

		markData.AddMember("Position", mark->position, domTree.GetAllocator());


		gradientData.PushBack(markData, domTree.GetAllocator());
	}

	moduleInfo.AddMember("ColorOverTime", gradientData, domTree.GetAllocator());


	// Get bezier curve data
	{
		rapidjson::Value curveData(rapidjson::kArrayType);

		curveData.PushBack(m_colorCurve[0], domTree.GetAllocator());
		curveData.PushBack(m_colorCurve[1], domTree.GetAllocator());
		curveData.PushBack(m_colorCurve[2], domTree.GetAllocator());
		curveData.PushBack(m_colorCurve[3], domTree.GetAllocator());

		moduleInfo.AddMember("ColorCurve", curveData, domTree.GetAllocator());
	}

	return moduleInfo;
}

bool EmitterColor::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	// For retrocompatibility //

	if (moduleInfo.HasMember("StartColor") && moduleInfo.HasMember("EndColor"))
	{
		m_colorOverTime.getMarks().clear();

		const auto& color = moduleInfo["StartColor"].GetArray();
		ImColor startColor = ImColor(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat(), color[3].GetFloat());

		m_colorOverTime.addMark(0.f, startColor);
		m_colorOverTime.addAlphaMark(0.f, startColor.Value.w);

		const auto& color2 = moduleInfo["EndColor"].GetArray();
		ImColor endColor = ImColor(color2[0].GetFloat(), color2[1].GetFloat(), color2[2].GetFloat(), color2[3].GetFloat());

		m_colorOverTime.addMark(1.f, endColor);
		m_colorOverTime.addAlphaMark(1.f, endColor.Value.w);

	}
	else if (moduleInfo.HasMember("ColorOverTime"))
	{
		// Get gradient colors //

		m_colorOverTime.getMarks().clear();

		const auto& marks = moduleInfo["ColorOverTime"].GetArray();

		for (const auto& mark : marks)
		{
			float position = mark["Position"].GetFloat();

			if (mark["IsAlphaMark"].GetBool())
			{
				float alpha = mark["Alpha"].GetFloat();

				m_colorOverTime.addAlphaMark(position, alpha);
			}
			else
			{
				const auto& color = mark["Color"].GetArray();
				ImColor markColor = ImColor(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());

				m_colorOverTime.addMark(position, markColor);
			}
		}
	}

	if (moduleInfo.HasMember("ColorCurve"))
	{
		const auto& curveArray = moduleInfo["ColorCurve"].GetArray();
		m_colorCurve[0] = curveArray[0].GetFloat();
		m_colorCurve[1] = curveArray[1].GetFloat();
		m_colorCurve[2] = curveArray[2].GetFloat();
		m_colorCurve[3] = curveArray[3].GetFloat();
	}

	return true;
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
