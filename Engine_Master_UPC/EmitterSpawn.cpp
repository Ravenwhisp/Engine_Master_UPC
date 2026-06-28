#include "Globals.h"
#include "EmitterSpawn.h"
#include "JsonArchive.h"

#include "Application.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterSpawn::update(EmitterInstance* instance)
{

	if (!m_looping && instance->getCurrentTime() > m_duration) return;

	float spawn = instance->getParticlesToSpawn();
	float deltaTime = instance->getParticleSystemComponent()->deltaTime();

	spawn += m_rateOverTime * deltaTime;
	spawn += m_rateOverDistance * instance->getParticleSystemComponent()->getDistance(); // on local position; this is Unity's behavior

	std::vector<unsigned int>& newParticles = instance->getNewParticles();
	ModuleParticleSystem* moduleParticleSystem = app->getModuleParticleSystem();

	while (spawn >= 1.0f)
	{
		int index = moduleParticleSystem->requestPoolSlot();
		if (index == -1)
			break;

		newParticles.push_back(index);

		spawn -= 1.0f;
	}

	spawn = spawn - static_cast<long>(spawn);

	instance->setParticlesToSpawn(spawn);

}

bool EmitterSpawn::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Spawn"))
	{
		parameterChanged |= ImGui::Checkbox("Looping##Spawn", &m_looping);
		parameterChanged |= ImGui::DragFloat("Duration##Spawn", &m_duration, 0.1f, 0.0f);

		parameterChanged |= ImGui::DragFloat("Rate over time##Spawn", &m_rateOverTime, 0.1f, 0.0f);
		parameterChanged |= ImGui::DragFloat("Rate over distance##Spawn", &m_rateOverDistance, 0.1f, 0.0f);
	}

	return parameterChanged;
}

void EmitterSpawn::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);
    archive.serialize(m_looping, "Looping");
    archive.serialize(m_duration, "Duration");
    archive.serialize(m_rateOverTime, "RateOverTime");
    archive.serialize(m_rateOverDistance, "RateOverDistance");
}
