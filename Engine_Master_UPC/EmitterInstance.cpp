#include "Globals.h"
#include "EmitterInstance.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"

#include "ModuleParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"
#include "CameraComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

#include "EmitterSpawn.h"

#include <algorithm>

namespace
{
	Vector3 getParticleSortCameraPosition()
	{
		Scene* scene = app->getModuleScene()->getScene();
		CameraComponent* gameCamera = scene != nullptr ? scene->getDefaultCamera() : nullptr;

		if (gameCamera != nullptr && gameCamera->getOwner() != nullptr && gameCamera->getOwner()->GetTransform() != nullptr)
		{
			return gameCamera->getOwner()->GetTransform()->getGlobalMatrix().Translation();
		}

		return app->getModuleCamera()->getPosition();
	}
}

EmitterInstance::EmitterInstance(ParticleEmitter* emitter, ParticleSystemComponent* owner) : m_emitter(emitter), m_owner(owner)
{
}

EmitterInstance::~EmitterInstance()
{
	freeParticleSlots();
}

void EmitterInstance::updateSpawnModule()
{
	m_emitter->getSpawnModule()->update(this);
}

void EmitterInstance::updateRemainingModules()
{
	// This assumes that spawn module has been already used for updating, AND that it was the first module
	
	std::vector<std::unique_ptr<ParticleModule>>& modules = m_emitter->getModules();

	for (auto module = modules.begin() + 1; module != modules.end(); ++module) 
	{
		(*module)->update(this);
	}

	manageNewParticles();

	auto& particlePool = app->getModuleParticleSystem()->getPool();
	const Vector3 cameraPosition = getParticleSortCameraPosition();
	for (auto& aliveParticle : m_aliveParticles)
	{
		aliveParticle.first = Vector3::DistanceSquared(particlePool[aliveParticle.second].position, cameraPosition);
	}

	// sort m_aliveParticles per distance (sqr) to the camera (first ones should be the farthest)
	std::sort(m_aliveParticles.begin(), m_aliveParticles.end(), [](std::pair<float, unsigned int> a, std::pair<float, unsigned int> b) 
	{
		return a.first > b.first;
	});

	updateAlivesOwnerData();

	m_currentTime += m_owner->deltaTime();
}

void EmitterInstance::reset() {

	freeParticleSlots();

	m_aliveParticles.clear();
	m_newParticles.clear();

	m_particlesToSpawn = 0.f;
	m_currentTime = 0.f;
}

void EmitterInstance::eraseIndexOnLocation(bool isNew, unsigned int vectorPosition)
{
	if (isNew) 
	{
		eraseBySwap(m_newParticles, vectorPosition);

		if (vectorPosition != m_newParticles.size()) 
		{
			// handle case where the erased element was not the last one (we swapped an index, so we have to update its owner data in the pool)

			app->getModuleParticleSystem()->updateOwnerData(m_newParticles[vectorPosition], true, vectorPosition);
		}
	}
	else 
	{
		eraseBySwap(m_aliveParticles, vectorPosition);

		if (vectorPosition != m_aliveParticles.size()) 
		{
			// handle case where the erased element was not the last one (we swapped an index, so we have to update its owner data in the pool)

			app->getModuleParticleSystem()->updateOwnerData(m_aliveParticles[vectorPosition].second, false, vectorPosition);
		}
	}
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
	for (auto particleIndex : m_newParticles) 
	{
		m_aliveParticles.push_back(std::make_pair(0.0f, particleIndex));
	}

	m_newParticles.clear();
}

void EmitterInstance::updateAlivesOwnerData()
{
	for (unsigned int i = 0; i < m_aliveParticles.size(); ++i) 
	{
		app->getModuleParticleSystem()->updateOwnerData(m_aliveParticles[i].second, false, i);
	}
}

void EmitterInstance::eraseBySwap(std::vector<unsigned int>& newParticles, unsigned int index)
{
	// maybe also consider case = 0 (would be swapWithFront() + pop_front(); we could even be smarter with cases for cache optimisations)
	if (index != newParticles.size() - 1) swapWithBack(newParticles, index);

	newParticles.pop_back();
}

void EmitterInstance::swapWithBack(std::vector<unsigned int>& newParticles, unsigned int index)
{
	unsigned int oldBack = newParticles.back();

	newParticles.back() = newParticles[index];
	newParticles[index] = oldBack;
}

void EmitterInstance::eraseBySwap(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index)
{
	// maybe also consider case = 0 (would be swapWithFront() + pop_front(); we could even be smarter with cases for cache optimisations)
	if (index != aliveParticles.size() - 1) swapWithBack(aliveParticles, index);

	aliveParticles.pop_back();
}

void EmitterInstance::swapWithBack(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index)
{
	std::pair<float, unsigned int> oldBack = aliveParticles.back();

	aliveParticles.back() = aliveParticles[index];
	aliveParticles[index] = oldBack;
}
