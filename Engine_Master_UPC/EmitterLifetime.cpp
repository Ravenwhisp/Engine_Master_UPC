#include "Globals.h"
#include "EmitterLifetime.h"
#include "JsonArchive.h"

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
		if (m_lifeTimeType == ParameterType::RANDOM_BETWEEN_TWO)
		{
			float scale = uniform_rand();
			particlePool[particleIndex].lifeTime = m_startLifeTime + (m_startLifeTime2 - m_startLifeTime) * scale;
		}
		else
		{
			particlePool[particleIndex].lifeTime = m_startLifeTime;
		}
	}
}

bool EmitterLifetime::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Lifetime"))
	{
		int parameterType = static_cast<int>(m_lifeTimeType);
		// Solo mostramos Constant y Random para Lifetime
		if (ImGui::Combo("Lifetime type##Lifetime", &parameterType, "Constant\0Random value between two\0"))
		{
			m_lifeTimeType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}

		if (m_lifeTimeType == ParameterType::CONSTANT)
		{
			parameterChanged |= ImGui::DragFloat("Initial lifetime##Lifetime", &m_startLifeTime, 0.1f, 0.0f);
		}
		else if (m_lifeTimeType == ParameterType::RANDOM_BETWEEN_TWO)
		{
			parameterChanged |= ImGui::DragFloat("Initial lifetime 1##Lifetime", &m_startLifeTime, 0.1f, 0.0f);
			parameterChanged |= ImGui::DragFloat("Initial lifetime 2##Lifetime", &m_startLifeTime2, 0.1f, 0.0f);
		}
	}

	return parameterChanged;
}

void EmitterLifetime::serialize(IArchive& archive)
{
	ParticleModule::serialize(archive);
	archive.serializeStringEnum(m_lifeTimeType, "LifeTimeType", ParameterTypeToString, StringToParameterType);
	archive.serialize(m_startLifeTime, "StartLifeTime");
	if (m_lifeTimeType == ParameterType::RANDOM_BETWEEN_TWO)
		archive.serialize(m_startLifeTime2, "StartLifeTime2");
}

bool EmitterLifetime::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartLifeTime")) {
		m_startLifeTime = moduleInfo["StartLifeTime"].GetFloat();
	}

	if(moduleInfo.HasMember("LifeTimeType")) {
		m_lifeTimeType = static_cast<ParameterType>(moduleInfo["LifeTimeType"].GetUint());

		if (m_lifeTimeType == ParameterType::RANDOM_BETWEEN_TWO && moduleInfo.HasMember("StartLifeTime2")) {
			m_startLifeTime2 = moduleInfo["StartLifeTime2"].GetFloat();
		}
	}
	else {
		m_lifeTimeType = ParameterType::CONSTANT; // Fallback para versiones anteriores
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
