#include "Globals.h"
#include "EmitterLifetime.h"

#include "EmitterInstance.h"
#include "Application.h"
#include "ModuleParticleSystem.h"

void EmitterLifetime::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles; // saves distance (sqr) to camera + index to pool
		particleData->getPoolAndAlives(particlePool, aliveParticles);
		
		// Dealing with already existing particles //
	
		unsigned int i = 0;
		while (i < aliveParticles->size()) 
		{
			unsigned int poolIndex = (*aliveParticles)[i].second;

			if (particlePool[poolIndex].lifeTime == 0) 
			{
				// Remove from alives

				aliveParticles->erase(aliveParticles->begin() + i);
				particleData->freePoolSlot(poolIndex); // mark poolIndex slot as free
			}
			else 
			{
				particlePool[poolIndex].lifeTime = std::max(0.f, particlePool[poolIndex].lifeTime - app->getModuleParticleSystem()->deltaTime());
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

bool EmitterLifetime::deserializeJSON(const rapidjson::Value& componentValue)
{
	return false;
}
