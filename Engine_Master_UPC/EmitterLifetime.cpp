#include "Globals.h"
#include "EmitterLifetime.h"

#include "EmitterInstance.h"
#include "Application.h"
#include "ModuleTime.h"

void EmitterLifetime::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<unsigned int, unsigned int>>* aliveParticles; // saves distance to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);
		
		// Dealing with already existing particles //
	
		unsigned int i = 0;
		while (i < aliveParticles->size()) 
		{
			unsigned int poolIndex = (*aliveParticles)[i].second;

			if (particlePool[poolIndex].lifeTime == 0) 
			{
				// Remove from alives

				particlePool[poolIndex].alive = false;
				aliveParticles->erase(aliveParticles->begin() + i);
				// We need an allocation method...
			}
			else 
			{
				particlePool[poolIndex].lifeTime = std::max(0.f, particlePool[poolIndex].lifeTime - app->getModuleTime()->deltaTime());
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
