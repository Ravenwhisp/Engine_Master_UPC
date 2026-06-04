#include "Globals.h"
#include "EmitterVelocity.h"
#include "Application.h"
#include "JsonArchive.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ModuleCamera.h"

void EmitterVelocity::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles; // saves distance to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();
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

bool EmitterVelocity::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Velocity"))
	{
		parameterChanged = ImGui::DragFloat("Initial velocity", &m_initialVelocity, 0.1f);
	}

	return parameterChanged;
}

void EmitterVelocity::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);
    archive.serialize(m_initialVelocity, "InitialVelocity");
}
