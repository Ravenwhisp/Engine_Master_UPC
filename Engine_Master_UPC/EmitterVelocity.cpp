#include "Globals.h"
#include "EmitterVelocity.h"

#include "EmitterInstance.h"
#include "Application.h"
#include "ModuleTime.h"
#include "ModuleCamera.h"

void EmitterVelocity::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<unsigned int, unsigned int>>* aliveParticles; // saves distance to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float deltaTime = app->getModuleTime()->deltaTime();
		Vector3 cameraPosition = app->getModuleCamera()->getPosition();

		for (auto& aliveParticle : *aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;
			Vector3 position = particlePool[poolIndex].position;

			position += (deltaTime * particlePool[poolIndex].velocity) * particlePool[poolIndex].movementDirection; // update position
			particlePool[poolIndex].position = position;

			aliveParticle.first = Vector3::Distance(cameraPosition, position); // update distance to camera

			// update particle velocity? (at least with linear interpolation)
			// ...
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].velocity = m_initialVelocity;
	}
}
