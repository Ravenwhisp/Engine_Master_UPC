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

bool EmitterSpawn::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Spawn"))
	{
		parameterChanged |= ImGui::Checkbox("Looping", &m_looping);
		parameterChanged |= ImGui::DragFloat("Duration", &m_duration, 0.1f, 0.0f);

		parameterChanged |= ImGui::DragFloat("Rate over time", &m_rateOverTime, 0.1f, 0.0f);
		parameterChanged |= ImGui::DragFloat("Rate over distance", &m_rateOverDistance, 0.1f, 0.0f);
	}

	return parameterChanged;
}

rapidjson::Value EmitterSpawn::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::SPAWN), domTree.GetAllocator());

	moduleInfo.AddMember("Looping", m_looping, domTree.GetAllocator());
	moduleInfo.AddMember("Duration", m_duration, domTree.GetAllocator());

	moduleInfo.AddMember("RateOverTime", m_rateOverTime, domTree.GetAllocator());
	moduleInfo.AddMember("RateOverDistance", m_rateOverDistance, domTree.GetAllocator());

	return moduleInfo;

}

bool EmitterSpawn::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("Looping")) {
		m_looping = moduleInfo["Looping"].GetBool();
	}

	if (moduleInfo.HasMember("Duration")) {
		m_duration = moduleInfo["Duration"].GetFloat();
	}


	if (moduleInfo.HasMember("RateOverTime")) {
		m_rateOverTime = moduleInfo["RateOverTime"].GetFloat();
	}

	if (moduleInfo.HasMember("RateOverDistance")) {
		m_rateOverDistance = moduleInfo["RateOverDistance"].GetFloat();
	}

	return true;
}
