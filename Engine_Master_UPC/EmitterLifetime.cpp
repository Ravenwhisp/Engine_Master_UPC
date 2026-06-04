#include "Globals.h"
#include "EmitterLifetime.h"
#include "JsonArchive.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterLifetime::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles; // saves distance (sqr) to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);
		
		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();

		unsigned int i = 0;
		while (i < aliveParticles->size()) 
		{
			unsigned int poolIndex = (*aliveParticles)[i].second;

			if (particlePool[poolIndex].lifeTime == 0.f) 
			{
				// Remove from alives
				// aliveParticles->erase(aliveParticles->begin() + i); <- Should not need to be brough back
				eraseBySwap(aliveParticles, i);
				particleData->freePoolSlot(poolIndex); // mark poolIndex slot as free
			}
			else 
			{
				particlePool[poolIndex].lifeTime = std::max(0.f, particlePool[poolIndex].lifeTime - deltaTime);
				++i;
			}
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles()) 
	{
		particlePool[particleIndex].lifeTime = m_startLifeTime;
	}
}

bool EmitterLifetime::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Lifetime"))
	{
		parameterChanged = ImGui::DragFloat("Initial lifetime", &m_startLifeTime, 0.1f, 0.0f);
	}

	return parameterChanged;
}

void EmitterLifetime::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);
    archive.serialize(m_startLifeTime, "StartLifeTime");
}

void EmitterLifetime::eraseBySwap(std::vector<std::pair<float, unsigned int>>* aliveParticles, unsigned int index)
{
	// maybe also consider case = 0 (would be swapWithFront() + pop_front(); we could even be smarter with cases for cache optimisations)
	if (index != aliveParticles->size()-1) swapWithBack(aliveParticles, index);

	aliveParticles->pop_back();
}

void EmitterLifetime::swapWithBack(std::vector<std::pair<float, unsigned int>>* aliveParticles, unsigned int index)
{
	std::pair<float, unsigned int> oldBack = aliveParticles->back();

	aliveParticles->back() = (*aliveParticles)[index];
	(*aliveParticles)[index] = oldBack;
}
