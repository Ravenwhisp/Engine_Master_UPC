#include "Globals.h"
#include "EmitterVelocity.h"

#include "EmitterInstance.h"
#include "Application.h"
#include "ModuleParticleSystem.h"
#include "ModuleCamera.h"

void EmitterVelocity::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles; // saves distance to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float deltaTime = app->getModuleParticleSystem()->deltaTime();
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
		parameterChanged = ImGui::DragFloat("Initial velocity", &m_initialVelocity, 0.1f, 0.0f);
	}

	return parameterChanged;
}

rapidjson::Value EmitterVelocity::getJSON(rapidjson::Document& domTree)
{
	return rapidjson::Value();
}

bool EmitterVelocity::deserializeJSON(const rapidjson::Value& componentValue)
{
	return false;
}
