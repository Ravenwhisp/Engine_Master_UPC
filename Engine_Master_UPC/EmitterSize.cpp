#include "Globals.h"
#include "EmitterSize.h"

//#include "GameObject.h"
//#include "Transform.h"

//#include "ParticleSystemComponent.h"
#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

void EmitterSize::update(EmitterInstance* particleData)
{
	Particle* particlePool;

	//Vector3 parentScale = particleData->getParticleSystemComponent()->getOwner()->GetTransform()->getScale();
	//Vector2 parentScale2D = Vector2(parentScale.x, parentScale.y); // because for now we are only going to use these
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles;
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : *aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float scale = particlePool[poolIndex].lifeTime / startLifetime;

			particlePool[poolIndex].scale = m_startScale * scale + m_endScale * (1.f - scale); // We need to use Bezier curves instead of this
		}
	}

	// Initialization for new ones //

	//Vector2 finalInitialScale = parentScale2D * m_startScale;
	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].scale = m_startScale;
	}
}

bool EmitterSize::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Size"))
	{

		// 2 scales to interpolate

		float scale[2] = { m_startScale.x, m_startScale.y};
		if (ImGui::DragFloat2("Starting scale", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_startScale = newScale;
			parameterChanged |= true;
		}

		scale[0] = m_endScale.x; scale[1] = m_endScale.y;
		if (ImGui::DragFloat2("End scale", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale = newScale;
			parameterChanged |= true;
		}

	}

	return parameterChanged;
}

rapidjson::Value EmitterSize::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::SIZE), domTree.GetAllocator());

	{
		rapidjson::Value scaleData(rapidjson::kArrayType);

		scaleData.PushBack(m_startScale.x, domTree.GetAllocator());
		scaleData.PushBack(m_startScale.y, domTree.GetAllocator());

		moduleInfo.AddMember("StartScale", scaleData, domTree.GetAllocator());
	}

	{
		rapidjson::Value scaleData(rapidjson::kArrayType);

		scaleData.PushBack(m_endScale.x, domTree.GetAllocator());
		scaleData.PushBack(m_endScale.y, domTree.GetAllocator());

		moduleInfo.AddMember("EndScale", scaleData, domTree.GetAllocator());
	}

	return moduleInfo;
}

bool EmitterSize::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartScale"))
	{
		const auto& scale = moduleInfo["StartScale"].GetArray();
		m_startScale = Vector2(scale[0].GetFloat(), scale[1].GetFloat());
	}

	if (moduleInfo.HasMember("EndScale"))
	{
		const auto& scale = moduleInfo["EndScale"].GetArray();
		m_endScale = Vector2(scale[0].GetFloat(), scale[1].GetFloat());
	}

	return true;
}
