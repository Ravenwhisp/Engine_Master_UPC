#include "Globals.h"
#include "EmitterSpawn.h"
#include "JsonArchive.h"

#include "Application.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterSpawn::update(EmitterInstance* instance)
{

	if (!m_looping && instance->getCurrentTime() > m_duration)
		return;

	float spawn = instance->getParticlesToSpawn();
	float deltaTime = instance->getParticleSystemComponent()->deltaTime();

	spawn += m_rateOverTime * deltaTime;
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

	spawn = spawn - static_cast<long>(spawn);

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

void EmitterSpawn::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);
    archive.serialize(m_looping, "Looping");
    archive.serialize(m_duration, "Duration");
    archive.serialize(m_rateOverTime, "RateOverTime");
    archive.serialize(m_rateOverDistance, "RateOverDistance");
}
