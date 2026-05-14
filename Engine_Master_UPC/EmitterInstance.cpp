#include "Globals.h"
#include "EmitterInstance.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ParticleEmitter.h"
#include "ModuleParticleSystem.h"

#include <algorithm>

EmitterInstance::EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner)
{
	initSlotManagement();
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

	m_currentTime += app->getModuleParticleSystem()->deltaTime();
}

void EmitterInstance::reset() {

	initSlotManagement();

	m_aliveParticles.clear();
	m_newParticles.clear();

	m_particlesToSpawn = 0.f;
	m_currentTime = 0.f;
}

int EmitterInstance::requestPoolSlot()
{
	if (m_firstFree == m_slots[m_firstFree]) return -1; // because the slot points to itself, which indicates used

	int slot = m_firstFree;

	m_firstFree = m_slots[slot]; // update first free, to point to the next one

	m_slots[slot] = slot; // mark as used

	return slot;
}

void EmitterInstance::freePoolSlot(unsigned int index)
{
	m_slots[index] = m_firstFree; // Set next free (because we are adding the index slot as the new first)

	m_firstFree = index; // Update first
}

void EmitterInstance::initSlotManagement()
{
	m_slots[MAX_PARTICLES - 1] = 0;

	for (unsigned int i = 0; i < MAX_PARTICLES - 1; ++i)
	{
		m_slots[i] = i + 1;
	}

	m_firstFree = 0;
}

void EmitterInstance::manageNewParticles()
{
	Vector3 cameraPosition = app->getModuleCamera()->getPosition();

	for (auto particleIndex : m_newParticles) 
	{
		float distanceSqrToCamera = Vector3::DistanceSquared(m_pool[particleIndex].position, cameraPosition);
		m_aliveParticles.push_back(std::make_pair(distanceSqrToCamera, particleIndex));
	}

	m_newParticles.clear();
}
