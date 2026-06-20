#include "Globals.h"
#include "EmitterVelocity.h"

#include "Application.h"
#include "ModuleCamera.h"
#include <imgui.h>
#include "imgui_bezier.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"


void EmitterVelocity::update(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();

		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();
		Vector3 cameraPosition = app->getModuleCamera()->getPosition();

		// Necesitamos el startLifetime para calcular en qué punto de la curva estamos
		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;
			Vector3 position = particlePool[poolIndex].position;

			
			if (m_velocityType == ParameterType::CURVE)
			{
				float scale = 1.f - (particlePool[poolIndex].lifeTime / startLifetime); // 0 al nacer, 1 al morir
				float bezierScale = ImGui::BezierValue(scale, m_velocityCurve);
				particlePool[poolIndex].velocity = (1.f - bezierScale) * m_initialVelocity + bezierScale * m_initialVelocity2;
			}

			position += (deltaTime * particlePool[poolIndex].velocity) * particlePool[poolIndex].movementDirection; // update position
			particlePool[poolIndex].position = position;

			aliveParticle.first = Vector3::Distance(cameraPosition, position); // update distance to camera
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		if (m_velocityType == ParameterType::RANDOM_BETWEEN_TWO)
		{
			float scale = uniform_rand();
			particlePool[particleIndex].velocity = (1.f - scale) * m_initialVelocity + scale * m_initialVelocity2;
		}
		else
		{
			particlePool[particleIndex].velocity = m_initialVelocity;
		}
	}
}
bool EmitterVelocity::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Velocity"))
	{
		parameterChanged = drawVelocityUI();
	}

	return parameterChanged;
}

bool EmitterVelocity::drawVelocityUI()
{
	bool parameterChanged = false;

	int parameterType = static_cast<int>(m_velocityType);
	if (ImGui::Combo("Velocity type", &parameterType, "Constant\0Random value between two\0Curve\0", static_cast<int>(ParameterType::TOTAL_TYPES)))
	{
		m_velocityType = static_cast<ParameterType>(parameterType);
		parameterChanged = true;
	}

	switch (m_velocityType)
	{
	case ParameterType::CONSTANT:
	{
		parameterChanged |= ImGui::DragFloat("Initial velocity", &m_initialVelocity, 0.1f);
		break;
	}
	case ParameterType::RANDOM_BETWEEN_TWO:
	{
		parameterChanged |= ImGui::DragFloat("Velocity 1", &m_initialVelocity, 0.1f);
		parameterChanged |= ImGui::DragFloat("Velocity 2", &m_initialVelocity2, 0.1f);
		break;
	}
	case ParameterType::CURVE:
	{
		parameterChanged |= ImGui::DragFloat("Velocity 1", &m_initialVelocity, 0.1f);
		parameterChanged |= ImGui::DragFloat("Velocity 2", &m_initialVelocity2, 0.1f);

		

		if (ImGui::Bezier("Curve", m_velocityCurve)) { parameterChanged = true; }

		if (ImGui::Button("Linear"))
		{
			m_velocityCurve[0] = 0.000f; m_velocityCurve[1] = 0.000f; m_velocityCurve[2] = 1.000f; m_velocityCurve[3] = 1.000f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseIn"))
		{
			m_velocityCurve[0] = 0.470f; m_velocityCurve[1] = 0.000f; m_velocityCurve[2] = 0.745f; m_velocityCurve[3] = 0.715f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseOut"))
		{
			m_velocityCurve[0] = 0.390f; m_velocityCurve[1] = 0.575f; m_velocityCurve[2] = 0.565f; m_velocityCurve[3] = 1.000f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseInOut"))
		{
			m_velocityCurve[0] = 0.445f; m_velocityCurve[1] = 0.050f; m_velocityCurve[2] = 0.550f; m_velocityCurve[3] = 0.950f;
			parameterChanged = true;
		}
		break;
	}
	}

	ImGui::Spacing();
	return parameterChanged;
}

rapidjson::Value EmitterVelocity::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::VELOCITY), domTree.GetAllocator());
	moduleInfo.AddMember("VelocityType", unsigned int(m_velocityType), domTree.GetAllocator());

	moduleInfo.AddMember("InitialVelocity", m_initialVelocity, domTree.GetAllocator());

	if (m_velocityType != ParameterType::CONSTANT)
	{
		moduleInfo.AddMember("InitialVelocity2", m_initialVelocity2, domTree.GetAllocator());

		if (m_velocityType == ParameterType::CURVE)
		{
			rapidjson::Value curveData(rapidjson::kArrayType);
			curveData.PushBack(m_velocityCurve[0], domTree.GetAllocator());
			curveData.PushBack(m_velocityCurve[1], domTree.GetAllocator());
			curveData.PushBack(m_velocityCurve[2], domTree.GetAllocator());
			curveData.PushBack(m_velocityCurve[3], domTree.GetAllocator());
			moduleInfo.AddMember("VelocityCurve", curveData, domTree.GetAllocator());
		}
	}

	return moduleInfo;
}

bool EmitterVelocity::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("InitialVelocity")) {
		m_initialVelocity = moduleInfo["InitialVelocity"].GetFloat();
	}

	if(moduleInfo.HasMember("VelocityType")) {
		m_velocityType = static_cast<ParameterType>(moduleInfo["VelocityType"].GetUint());

		if (moduleInfo.HasMember("InitialVelocity2")) {
			m_initialVelocity2 = moduleInfo["InitialVelocity2"].GetFloat();
		}

		if (m_velocityType == ParameterType::CURVE && moduleInfo.HasMember("VelocityCurve")) {
			const auto& curveArray = moduleInfo["VelocityCurve"].GetArray();
			m_velocityCurve[0] = curveArray[0].GetFloat();
			m_velocityCurve[1] = curveArray[1].GetFloat();
			m_velocityCurve[2] = curveArray[2].GetFloat();
			m_velocityCurve[3] = curveArray[3].GetFloat();
		}
	}
	else {
		m_velocityType = ParameterType::CONSTANT; 
	}
	return true;
}
