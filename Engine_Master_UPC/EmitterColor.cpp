#include "Globals.h"
#include "EmitterColor.h"
#include "JsonArchive.h"

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

void EmitterColor::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);

    DirectX::SimpleMath::Color startColor(m_startColor.x, m_startColor.y, m_startColor.z, m_startColor.w);
    archive.serialize(startColor, "StartColor");
    if (archive.mode() == ArchiveMode::Input)
    {
        m_startColor.x = startColor.x;
        m_startColor.y = startColor.y;
        m_startColor.z = startColor.z;
        m_startColor.w = startColor.w;
    }

    DirectX::SimpleMath::Color endColor(m_endColor.x, m_endColor.y, m_endColor.z, m_endColor.w);
    archive.serialize(endColor, "EndColor");
    if (archive.mode() == ArchiveMode::Input)
    {
        m_endColor.x = endColor.x;
        m_endColor.y = endColor.y;
        m_endColor.z = endColor.z;
        m_endColor.w = endColor.w;
    }
}
