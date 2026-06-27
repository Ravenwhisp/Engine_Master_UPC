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

		// Necesitamos el startLifetime para calcular en qu� punto de la curva estamos
		// float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime(); <- SAME ISSUE AS IN OTHER MODULES

		for (auto& aliveParticle : aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;
			Vector3 position = particlePool[poolIndex].position;

			
			if (m_velocityType == ParameterType::CURVE)
			{
				float scale = 1.f - (particlePool[poolIndex].lifeTime / particlePool[poolIndex].startLifeTime); // 0 al nacer, 1 al morir
				float bezierScale = ImGui::BezierValue(scale, m_velocityCurve);
				particlePool[poolIndex].velocity = m_initialVelocity + (m_initialVelocity2 - m_initialVelocity) * bezierScale;
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
			particlePool[particleIndex].velocity = m_initialVelocity + (m_initialVelocity2 - m_initialVelocity) * scale;
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
	if (ImGui::Combo("Velocity type##Velocity", &parameterType, "Constant\0Random value between two\0Curve\0", static_cast<int>(ParameterType::TOTAL_TYPES)))
	{
		m_velocityType = static_cast<ParameterType>(parameterType);
		parameterChanged = true;
	}

	switch (m_velocityType)
	{
	case ParameterType::CONSTANT:
	{
		parameterChanged |= ImGui::DragFloat("Initial velocity##Velocity", &m_initialVelocity, 0.1f);
		break;
	}
	case ParameterType::RANDOM_BETWEEN_TWO:
	{
		parameterChanged |= ImGui::DragFloat("Velocity 1##Velocity", &m_initialVelocity, 0.1f);
		parameterChanged |= ImGui::DragFloat("Velocity 2##Velocity", &m_initialVelocity2, 0.1f);
		break;
	}
	case ParameterType::CURVE:
	{
		parameterChanged |= ImGui::DragFloat("Velocity 1##Velocity", &m_initialVelocity, 0.1f);
		parameterChanged |= ImGui::DragFloat("Velocity 2##Velocity", &m_initialVelocity2, 0.1f);

		

		if (ImGui::Bezier("Curve##Velocity", m_velocityCurve)) { parameterChanged = true; }

		if (ImGui::Button("Linear##Velocity"))
		{
			m_velocityCurve[0] = 0.000f; m_velocityCurve[1] = 0.000f; m_velocityCurve[2] = 1.000f; m_velocityCurve[3] = 1.000f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseIn##Velocity"))
		{
			m_velocityCurve[0] = 0.470f; m_velocityCurve[1] = 0.000f; m_velocityCurve[2] = 0.745f; m_velocityCurve[3] = 0.715f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseOut##Velocity"))
		{
			m_velocityCurve[0] = 0.390f; m_velocityCurve[1] = 0.575f; m_velocityCurve[2] = 0.565f; m_velocityCurve[3] = 1.000f;
			parameterChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("EaseInOut##Velocity"))
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

void EmitterVelocity::serialize(IArchive& archive)
{
	ParticleModule::serialize(archive);
	archive.serializeStringEnum(m_velocityType, "VelocityType", ParameterTypeToString, StringToParameterType);
	archive.serialize(m_initialVelocity, "InitialVelocity");

	if (m_velocityType != ParameterType::CONSTANT)
	{
		archive.serialize(m_initialVelocity2, "InitialVelocity2");

		if (m_velocityType == ParameterType::CURVE)
		{
			uint32_t curveCount = 4;
			archive.beginArray(curveCount, "VelocityCurve");
			for (int i = 0; i < 4; ++i)
				archive.serialize(m_velocityCurve[i], "");
			archive.endArray();
		}
	}
}
