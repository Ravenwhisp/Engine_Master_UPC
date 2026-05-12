#include "Globals.h"
#include "EmitterInstance.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ParticleEmitter.h"

EmitterInstance::EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner)
{
	m_slots[MAX_PARTICLES - 1] = 0;

	for (unsigned int i = 0; i < MAX_PARTICLES - 1; ++i)
	{
		m_slots[i] = i + 1;
	}
}

void EmitterInstance::updateModules()
{
	std::vector<std::unique_ptr<ParticleModule>> modules = m_emitter->getModules();

	for (auto& module : modules) 
	{
		module->update(this);
	}

	manageNewParticles();

	// sort m_aliveParticles per distance (sqr) to the camera (first ones should be the farthest)
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
