#include "Globals.h"
#include "EmitterColor.h"

#include "Application.h"
#include "ModuleInput.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

EmitterColor::EmitterColor() : ParticleModule(ParticleModuleType::COLOR) {

	
	m_colorsOverTime.getMarks().clear(); // because it has default values that we don't want

	m_colorsOverTime.addMark(0.f, ImColor(1.f, 1.f, 1.f, 1.f));
	m_colorsOverTime.addMark(1.f, ImColor(1.f, 1.f, 1.f, 1.f));
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

			float scale = particlePool[poolIndex].lifeTime / startLifetime;

			particlePool[poolIndex].colorAndAlpha = m_startColor*scale + m_endColor*(1.f-scale); // We need to use Bezier curves instead of this
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].colorAndAlpha = m_startColor;
	}
}

bool EmitterColor::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Color")) 
	{

		// 2 colors to interpolate

		float color[4] = { m_startColor.x, m_startColor.y, m_startColor.z, m_startColor.w };
		if (ImGui::ColorEdit4("Starting color", color))
		{
			Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
			m_startColor = newColor;
			parameterChanged = true;
		}

		color[0] = m_endColor.x; color[1] = m_endColor.y; color[2] = m_endColor.z; color[3] = m_endColor.w;
		if (ImGui::ColorEdit4("End color", color))
		{
			Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
			m_endColor = newColor;
			parameterChanged = true;
		}

		
		// Color gradient editor
		if (ImGui::GradientButton(&m_colorsOverTime)) {

			ImGui::OpenPopup("GradientEditorPopup");
		}

		if (ImGui::BeginPopup("GradientEditorPopup")) {
			
			//ImGui::PushID("GradientID"); // to avoid conflicting index error
			//size_t beforeMarkers = m_colorsOverTime.getMarks().size();

			parameterChanged |= ImGui::GradientEditor(&m_colorsOverTime, m_draggingMark, m_selectedMark);

			if (app->getModuleInput()->isKeyJustPressed(Keyboard::Keys::Delete) && m_selectedMark != nullptr)
			{
				m_colorsOverTime.removeMark(m_selectedMark);
			}

			//ImGui::PopID(); // (same, corresponding)

			ImGui::EndPopup();
		}

	}

	return parameterChanged;
}

rapidjson::Value EmitterColor::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::COLOR), domTree.GetAllocator());

	{
		rapidjson::Value colorData(rapidjson::kArrayType);

		colorData.PushBack(m_startColor.x, domTree.GetAllocator());
		colorData.PushBack(m_startColor.y, domTree.GetAllocator());
		colorData.PushBack(m_startColor.z, domTree.GetAllocator());
		colorData.PushBack(m_startColor.w, domTree.GetAllocator());

		moduleInfo.AddMember("StartColor", colorData, domTree.GetAllocator());
	}

	{
		rapidjson::Value colorData(rapidjson::kArrayType);

		colorData.PushBack(m_endColor.x, domTree.GetAllocator());
		colorData.PushBack(m_endColor.y, domTree.GetAllocator());
		colorData.PushBack(m_endColor.z, domTree.GetAllocator());
		colorData.PushBack(m_endColor.w, domTree.GetAllocator());

		moduleInfo.AddMember("EndColor", colorData, domTree.GetAllocator());
	}

	return moduleInfo;
}

bool EmitterColor::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartColor"))
	{
		const auto& color = moduleInfo["StartColor"].GetArray();
		m_startColor = Vector4(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat(), color[3].GetFloat());
	}

	if (moduleInfo.HasMember("EndColor"))
	{
		const auto& color = moduleInfo["EndColor"].GetArray();
		m_endColor = Vector4(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat(), color[3].GetFloat());
	}

	return true;
}
