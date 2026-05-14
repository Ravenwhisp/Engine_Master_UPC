#include "Globals.h"
#include "EmitterColor.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

#include "imgui_color_gradient.h"



void EmitterColor::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles;
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : *aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float scale = particlePool[poolIndex].lifeTime / startLifetime;

			particlePool[poolIndex].colorAndAlpha = m_creationColor * (m_startColor*scale + m_endColor*(1.f-scale)); // We need to use Bezier curves instead of this
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].colorAndAlpha = m_creationColor;
	}
}

bool EmitterColor::drawUi()
{
	bool parameterChanged = false;

	
	if (ImGui::CollapsingHeader("Color")) 
	{
		
		// Creation color //

		float color[4] = { m_creationColor.x, m_creationColor.y, m_creationColor.z, m_creationColor.w};
		if (ImGui::ColorEdit4("Creation color", color))
		{
			Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]); 
			m_creationColor = newColor;
			parameterChanged |= true; 
		}

		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

		// 2 colors to interpotate //

		color[0] = m_startColor.x; color[1] = m_startColor.y; color[2] = m_startColor.z; color[3] = m_startColor.w;
		if (ImGui::ColorEdit4("Starting color", color))
		{
			Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
			m_startColor = newColor;
			parameterChanged |= true;
		}

		color[0] = m_endColor.x; color[1] = m_endColor.y; color[2] = m_endColor.z; color[3] = m_endColor.w;
		if (ImGui::ColorEdit4("End color", color))
		{
			Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
			m_endColor = newColor;
			parameterChanged |= true;
		}

		/* // TO EXPLORE LATER
		ImGradient gradient;
		ImGradientMark* draggingMark = nullptr;
		ImGradientMark* selectedMark = nullptr;
		if (ImGui::GradientButton(&gradient))
		{
			//set show editor flag to true/false
		}
		*/

	}

	return parameterChanged;
}

rapidjson::Value EmitterColor::getJSON(rapidjson::Document& domTree)
{
	return rapidjson::Value();
}

bool EmitterColor::deserializeJSON(const rapidjson::Value& componentValue)
{
	return false;
}
