#include "Globals.h"
#include "EmitterLifetime.h"

#include "Application.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterLifetime::update(EmitterInstance* particleData)
{
	ModuleParticleSystem* moduleParticleSystem = app->getModuleParticleSystem();

	auto& particlePool = moduleParticleSystem->getPool();
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();
		
		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();

		unsigned int i = 0;
		while (i < aliveParticles.size()) 
		{
			unsigned int poolIndex = aliveParticles[i].second;

			if (particlePool[poolIndex].lifeTime == 0.f) 
			{
				// Remove from alives
				// aliveParticles->erase(aliveParticles->begin() + i); <- Should not need to be brough back
				eraseBySwap(aliveParticles, i);
				moduleParticleSystem->freePoolSlot(poolIndex); // mark poolIndex slot as free
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

rapidjson::Value EmitterLifetime::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::LIFETIME), domTree.GetAllocator());

	moduleInfo.AddMember("StartLifeTime", m_startLifeTime, domTree.GetAllocator());

	return moduleInfo;
}

bool EmitterLifetime::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartLifeTime")) {
		m_startLifeTime = moduleInfo["StartLifeTime"].GetFloat();
	}

	return true;
}

void EmitterLifetime::eraseBySwap(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index)
{
	// maybe also consider case = 0 (would be swapWithFront() + pop_front(); we could even be smarter with cases for cache optimisations)
	if (index != aliveParticles.size()-1) swapWithBack(aliveParticles, index);

	aliveParticles.pop_back();
}

void EmitterLifetime::swapWithBack(std::vector<std::pair<float, unsigned int>>& aliveParticles, unsigned int index)
{
	std::pair<float, unsigned int> oldBack = aliveParticles.back();

	aliveParticles.back() = aliveParticles[index];
	aliveParticles[index] = oldBack;
}
