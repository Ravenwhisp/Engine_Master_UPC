#include "Globals.h"
#include "EmitterSize.h"
#include "JsonArchive.h"

//#include "GameObject.h"
//#include "Transform.h"

//#include "ParticleSystemComponent.h"
#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

void EmitterSize::update(EmitterInstance* particleData)
{
	Particle* particlePool;

	//Vector3 parentScale = particleData->getParticleSystemComponent()->getOwner()->GetTransform()->getScale();
	//Vector2 parentScale2D = Vector2(parentScale.x, parentScale.y); // because for now we are only going to use these
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles;
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : *aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float scale = particlePool[poolIndex].lifeTime / startLifetime;

			particlePool[poolIndex].scale = m_startScale * scale + m_endScale * (1.f - scale); // We need to use Bezier curves instead of this
		}
	}

	// Initialization for new ones //

	//Vector2 finalInitialScale = parentScale2D * m_startScale;
	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].scale = m_startScale;
	}
}

bool EmitterSize::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Size"))
	{

		// 2 scales to interpolate

		float scale[2] = { m_startScale.x, m_startScale.y};
		if (ImGui::DragFloat2("Starting scale", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_startScale = newScale;
			parameterChanged |= true;
		}

		scale[0] = m_endScale.x; scale[1] = m_endScale.y;
		if (ImGui::DragFloat2("End scale", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale = newScale;
			parameterChanged |= true;
		}

	}

	return parameterChanged;
}

void EmitterSize::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);

    DirectX::SimpleMath::Vector3 startScale(m_startScale.x, m_startScale.y, 0.0f);
    archive.serialize(startScale, "StartScale");
    if (archive.mode() == ArchiveMode::Input)
    {
        m_startScale.x = startScale.x;
        m_startScale.y = startScale.y;
    }

    DirectX::SimpleMath::Vector3 endScale(m_endScale.x, m_endScale.y, 0.0f);
    archive.serialize(endScale, "EndScale");
    if (archive.mode() == ArchiveMode::Input)
    {
        m_endScale.x = endScale.x;
        m_endScale.y = endScale.y;
    }
}
