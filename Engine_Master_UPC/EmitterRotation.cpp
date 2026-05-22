#include "Globals.h"
#include "EmitterRotation.h"

#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"

void EmitterRotation::update(EmitterInstance* particleData)
{
	Particle* particlePool;
	{
		std::vector<std::pair<float, unsigned int>>* aliveParticles;
		particleData->getPoolAndAlives(particlePool, aliveParticles);

		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();

		for (auto& aliveParticle : *aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float rotationVelocity = particlePool[poolIndex].rotationVelocity;
			float lastRotation = particlePool[poolIndex].rotationZ;

			particlePool[poolIndex].rotationZ = getNormalizedAngle(lastRotation + deltaTime*rotationVelocity);
		}
	}

	// Initialization for new ones //

	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].rotationZ = m_startRotation;

		if (uniform_rand() + 0.001f <= m_flipRotationLikelihood) // + 0.001f so that m_flipRotationLikelihood == 0 can be used as no flip case  
		{
			particlePool[particleIndex].rotationVelocity = -m_angularVelocity;
		}
		else particlePool[particleIndex].rotationVelocity = m_angularVelocity;
	}
}

bool EmitterRotation::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Rotation"))
	{
		// Maybe we are better off having separate variables in degrees, and just copying the conversion to the radians ones...

		float rotationDegrees = XMConvertToDegrees(m_startRotation);
		if (ImGui::DragFloat("Start rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f)) 
		{
			m_startRotation = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}

		rotationDegrees = XMConvertToDegrees(m_angularVelocity);
		if (ImGui::DragFloat("Angular velocity", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}

		parameterChanged |= ImGui::DragFloat("Flip rotation", &m_flipRotationLikelihood, 0.1f, 0.0f, 1.0f);
	}

	return parameterChanged;
}

rapidjson::Value EmitterRotation::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::ROTATION), domTree.GetAllocator());

	moduleInfo.AddMember("StartRotation", m_startRotation, domTree.GetAllocator());

	moduleInfo.AddMember("AngularVelocity", m_angularVelocity, domTree.GetAllocator());
	moduleInfo.AddMember("FlipRotation", m_flipRotationLikelihood, domTree.GetAllocator());

	return moduleInfo;
}

bool EmitterRotation::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartRotation"))
	{
		m_startRotation = moduleInfo["StartRotation"].GetFloat();
	}

	if (moduleInfo.HasMember("AngularVelocity"))
	{
		m_angularVelocity = moduleInfo["AngularVelocity"].GetFloat();
	}
	if (moduleInfo.HasMember("FlipRotation"))
	{
		m_flipRotationLikelihood = moduleInfo["FlipRotation"].GetFloat();
	}

	return true;
}

inline float EmitterRotation::getNormalizedAngle(float angle)
{
	while (angle >= XM_2PI) angle -= XM_2PI;
	while (angle < 0) angle += XM_2PI;

	return angle;
}
