#include "Globals.h"
#include "EmitterSpawn.h"

#include "Application.h"
#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterSpawn::update(EmitterInstance* particleData)
{
	if (!m_looping && particleData->getCurrentTime() > m_duration) return;


	float particlesToSpawn = particleData->getParticlesToSpawn();

	particlesToSpawn += m_rateOverTime * app->getModuleParticleSystem()->deltaTime();
	particlesToSpawn += m_rateOverDistance * particleData->getParticleSystemComponent()->getDistance(); // maybe better on GameObject?

	if (particlesToSpawn < 1.f) return;

	Particle* particlePool = particleData->getParticlePool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	// We instantiate all particles that can be instantiated (natural part of particlesToSpawn)
	while (particlesToSpawn >= 1.f)
	{
		int index = particleData->requestPoolSlot();
		if (index == -1) break; // => no more slots for particles (we will have to do something else)

		newParticles.push_back(index);

		--particlesToSpawn;
	}

	// (For now we will not do anything if we run out of space; but the idea would be REPLACING the oldest particles as new ones)
	particlesToSpawn = particlesToSpawn - static_cast<long>(particlesToSpawn);
}
