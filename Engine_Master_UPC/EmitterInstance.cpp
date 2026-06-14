#include "Globals.h"
#include "EmitterInstance.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"

#include <algorithm>

EmitterInstance::EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner)
{
}

EmitterInstance::~EmitterInstance()
{
	freeParticleSlots();
}

void EmitterInstance::updateModules()
{
	
	std::vector<std::unique_ptr<ParticleModule>>& modules = m_emitter->getModules();

	for (auto& module : modules) 
	{
		module->update(this);
	}

	manageNewParticles();

	// sort m_aliveParticles per distance (sqr) to the camera (first ones should be the farthest)
	std::sort(m_aliveParticles.begin(), m_aliveParticles.end(), [](std::pair<float, unsigned int> a, std::pair<float, unsigned int> b) 
	{
		return a.first > b.first;
	});

	m_currentTime += m_owner->deltaTime();
}

void EmitterInstance::reset() {

	freeParticleSlots();

	m_aliveParticles.clear();
	m_newParticles.clear();

	m_particlesToSpawn = 0.f;
	m_currentTime = 0.f;
}

void EmitterInstance::freeParticleSlots()
{
	ModuleParticleSystem* moduleParticleSystem = app->getModuleParticleSystem();

	for (std::pair<float, unsigned int>& particleData : m_aliveParticles) 
	{
		moduleParticleSystem->freePoolSlot(particleData.second);
	}

	for (unsigned int index : m_newParticles)
	{
		moduleParticleSystem->freePoolSlot(index);
	}

}

void EmitterInstance::manageNewParticles()
{
	auto& pool = app->getModuleParticleSystem()->getPool();
	Vector3 cameraPosition = app->getModuleCamera()->getPosition();

	for (auto particleIndex : m_newParticles) 
	{
		float distanceSqrToCamera = Vector3::DistanceSquared(pool[particleIndex].position, cameraPosition);
		m_aliveParticles.push_back(std::make_pair(distanceSqrToCamera, particleIndex));
	}

	m_newParticles.clear();
}
