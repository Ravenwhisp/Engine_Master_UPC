#include "Globals.h"
#include "EmitterColor.h"

#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"



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
