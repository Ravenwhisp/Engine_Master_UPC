#include "Globals.h"
#include "EmitterSpawn.h"

#include "Application.h"
#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterSpawn::update(EmitterInstance* instance)
{

	if (!m_looping && instance->getCurrentTime() > m_duration)
		return;

	float spawn = instance->getParticlesToSpawn();
	float dt = app->getModuleParticleSystem()->deltaTime();

	spawn += m_rateOverTime * dt;
	spawn += m_rateOverDistance * instance->getParticleSystemComponent()->getDistance();

	std::vector<unsigned int>& newParticles = instance->getNewParticles();

	while (spawn >= 1.0f)
	{
		int index = instance->requestPoolSlot();
		if (index == -1)
			break;

		newParticles.push_back(index);

		spawn -= 1.0f;
	}

	instance->setParticlesToSpawn(spawn);

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
