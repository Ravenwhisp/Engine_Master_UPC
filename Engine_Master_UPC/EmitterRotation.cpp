#include "Globals.h"
#include "EmitterRotation.h"

#include "Application.h"
#include "imgui_bezier.h"

#include "ModuleParticleSystem.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"


void EmitterRotation::update(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();

		// Dealing with already existing particles //

		if (m_angularVelocityType == ParameterType::CURVE) 
		{
			updateAlivesRotationWithCurve(particlePool, aliveParticles, particleData->getParticleSystemComponent()->deltaTime(),
				particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime());

		}
		else 
		{
			updateAlivesRotationFixed(particlePool, aliveParticles, particleData->getParticleSystemComponent()->deltaTime());
		}
	}

	// Initialization for new ones //

	if (m_angularVelocityType == ParameterType::RANDOM_BETWEEN_TWO) 
	{
		setNewParticlesVelocityWithRange(particlePool, particleData->getNewParticles());
	}
	else
	{
		setNewParticlesVelocityFixed(particlePool, particleData->getNewParticles());
	}
}

bool EmitterRotation::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Rotation"))
	{
		// Maybe we are better off having separate variables in degrees for the UI, and just copying the conversion to the radians ones...

		parameterChanged = drawStartRotationUI();

		parameterChanged |= drawAngularVelocityUI();

		parameterChanged |= ImGui::DragFloat("Flip rotation##Rotation", &m_flipRotationLikelihood, 0.1f, 0.0f, 1.0f);
	}

	return parameterChanged;
}

rapidjson::Value EmitterRotation::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::ROTATION), domTree.GetAllocator());

	moduleInfo.AddMember("RotationType", unsigned int(m_startRotationType), domTree.GetAllocator());
	moduleInfo.AddMember("StartRotation", m_startRotation, domTree.GetAllocator());

	if (m_startRotationType != ParameterType::CONSTANT)
	{
		moduleInfo.AddMember("StartRotation2", m_startRotation2, domTree.GetAllocator());
	}


	moduleInfo.AddMember("VelocityType", unsigned int(m_angularVelocityType), domTree.GetAllocator());
	moduleInfo.AddMember("AngularVelocity", m_angularVelocity, domTree.GetAllocator());

	if (m_angularVelocityType != ParameterType::CONSTANT)
	{
		moduleInfo.AddMember("AngularVelocity2", m_angularVelocity2, domTree.GetAllocator());

		if (m_angularVelocityType == ParameterType::CURVE) 
		{
			rapidjson::Value curveData(rapidjson::kArrayType);

			curveData.PushBack(m_angularVelocityCurve[0], domTree.GetAllocator());
			curveData.PushBack(m_angularVelocityCurve[1], domTree.GetAllocator());
			curveData.PushBack(m_angularVelocityCurve[2], domTree.GetAllocator());
			curveData.PushBack(m_angularVelocityCurve[3], domTree.GetAllocator());

			moduleInfo.AddMember("VelocityCurve", curveData, domTree.GetAllocator());
		}
	}

	moduleInfo.AddMember("FlipRotation", m_flipRotationLikelihood, domTree.GetAllocator());

	return moduleInfo;
}

bool EmitterRotation::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("StartRotation"))
	{
		m_startRotation = moduleInfo["StartRotation"].GetFloat();
	}

	if (moduleInfo.HasMember("RotationType")) // for versions that support choose random between 2 values start rotations
	{
		unsigned int rotationTypeUInt = moduleInfo["RotationType"].GetUint();
		ParameterType rotationType = static_cast<ParameterType>(rotationTypeUInt);

		switch (rotationType) {

		case ParameterType::CONSTANT:

			m_startRotationType = ParameterType::CONSTANT;

			break;

		case ParameterType::RANDOM_BETWEEN_TWO:

			m_startRotationType = ParameterType::RANDOM_BETWEEN_TWO;

			if (moduleInfo.HasMember("StartRotation2"))
			{
				m_startRotation2 = moduleInfo["StartRotation2"].GetFloat();
			}

			// (curve case would go here)
		}

	}
	else m_startRotationType = ParameterType::CONSTANT; // we recreate state corresponding to previous version (PROBABLY UNNEEDED, SINCE THE MODULE IS FRESHLY INSTANTIATED)

	if (moduleInfo.HasMember("AngularVelocity"))
	{
		m_angularVelocity = moduleInfo["AngularVelocity"].GetFloat();
	}

	if (moduleInfo.HasMember("VelocityType")) // for versions that support curves, choose random between 2 values angular velocities 
	{
		unsigned int velocityTypeUInt = moduleInfo["VelocityType"].GetUint();
		ParameterType velocityType = static_cast<ParameterType>(velocityTypeUInt);

		switch (velocityType) {

		case ParameterType::CONSTANT:

			m_angularVelocityType = ParameterType::CONSTANT;

			break;

		case ParameterType::RANDOM_BETWEEN_TWO:

			m_angularVelocityType = ParameterType::RANDOM_BETWEEN_TWO;

			if (moduleInfo.HasMember("AngularVelocity2"))
			{
				m_angularVelocity2 = moduleInfo["AngularVelocity2"].GetFloat();
			}

			break;

		case ParameterType::CURVE:

			m_angularVelocityType = ParameterType::CURVE;

			if (moduleInfo.HasMember("AngularVelocity2"))
			{
				m_angularVelocity2 = moduleInfo["AngularVelocity2"].GetFloat();
			}

			if (moduleInfo.HasMember("VelocityCurve"))
			{
				const auto& curveArray = moduleInfo["VelocityCurve"].GetArray();
				m_angularVelocityCurve[0] = curveArray[0].GetFloat();
				m_angularVelocityCurve[1] = curveArray[1].GetFloat();
				m_angularVelocityCurve[2] = curveArray[2].GetFloat();
				m_angularVelocityCurve[3] = curveArray[3].GetFloat();
			}
		}

	}else m_angularVelocityType = ParameterType::CONSTANT; // we recreate state corresponding to previous version


	if (moduleInfo.HasMember("FlipRotation"))
	{
		m_flipRotationLikelihood = moduleInfo["FlipRotation"].GetFloat();
	}

	return true;
}

bool EmitterRotation::drawStartRotationUI()
{
	bool parameterChanged = false;

	// Type selection combo (COULD BE REPLACED WITH SOMETHING SMALLER?)
	{
		int parameterType = static_cast<int>(m_startRotationType);
		if (ImGui::Combo("Start rotation type##Rotation", &parameterType, "Constant\0Random value between two\0", static_cast<int>(ParameterType::TOTAL_TYPES)))
		{
			m_startRotationType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}
	}

	switch (m_startRotationType) {

	case ParameterType::CONSTANT:

	{
		float rotationDegrees = XMConvertToDegrees(m_startRotation);
		if (ImGui::DragFloat("Start rotation##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_startRotation = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}
	}
	break;


	case ParameterType::RANDOM_BETWEEN_TWO:

	{
		float rotationDegrees = XMConvertToDegrees(m_startRotation);
		if (ImGui::DragFloat("Start rotation 1##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_startRotation = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}

		rotationDegrees = XMConvertToDegrees(m_startRotation2);
		if (ImGui::DragFloat("Start rotation 2##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_startRotation2 = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}
	}

	// (Curve case will be added if needed) 
	}

	ImGui::Spacing();

	return parameterChanged;
}

bool EmitterRotation::drawAngularVelocityUI()
{
	bool parameterChanged = false;

	// Type selection combo (COULD BE REPLACED WITH SOMETHING SMALLER?)
	{
		int parameterType = static_cast<int>(m_angularVelocityType);
		if (ImGui::Combo("Velocity type##Rotation", &parameterType, "Constant\0Random value between two\0Curve\0", static_cast<int>(ParameterType::TOTAL_TYPES)))
		{
			m_angularVelocityType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}
	}

	switch (m_angularVelocityType) {

	case ParameterType::CONSTANT:

	{
		float rotationDegrees = XMConvertToDegrees(m_angularVelocity);
		if (ImGui::DragFloat("Angular velocity##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}
	}
		break;


	case ParameterType::RANDOM_BETWEEN_TWO:

	{
		float rotationDegrees = XMConvertToDegrees(m_angularVelocity);
		if (ImGui::DragFloat("Angular velocity 1##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}

		rotationDegrees = XMConvertToDegrees(m_angularVelocity2);
		if (ImGui::DragFloat("Angular velocity 2##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity2 = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}
	}
		break;


	case ParameterType::CURVE:

		// 1. Range of values (for that, we use the 2 constants)
	{
		float rotationDegrees = XMConvertToDegrees(m_angularVelocity);
		if (ImGui::DragFloat("Angular velocity 1##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}

		rotationDegrees = XMConvertToDegrees(m_angularVelocity2);
		if (ImGui::DragFloat("Angular velocity 2##Rotation", &rotationDegrees, 0.1f, -360.0f, 360.0f))
		{
			m_angularVelocity2 = XMConvertToRadians(rotationDegrees);

			parameterChanged = true;
		}
	}

		// 2. Curve (between 0 and 1)

		if (ImGui::Bezier("Curve##Rotation", m_angularVelocityCurve))
		{
			parameterChanged = true;
		}

		// We add some buttons to quickly change to predefined setups
		if (ImGui::Button("Linear##Rotation"))
		{ 
			m_angularVelocityCurve[0] = 0.000f; m_angularVelocityCurve[1] = 0.000f; m_angularVelocityCurve[2] = 1.000f; m_angularVelocityCurve[3] = 1.000f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseIn##Rotation"))
		{
			m_angularVelocityCurve[0] = 0.470f; m_angularVelocityCurve[1] = 0.000f; m_angularVelocityCurve[2] = 0.745f; m_angularVelocityCurve[3] = 0.715f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseOut##Rotation"))
		{
			m_angularVelocityCurve[0] = 0.390f; m_angularVelocityCurve[1] = 0.575f; m_angularVelocityCurve[2] = 0.565f; m_angularVelocityCurve[3] = 1.000f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseInOut##Rotation"))
		{
			m_angularVelocityCurve[0] = 0.445f; m_angularVelocityCurve[1] = 0.050f; m_angularVelocityCurve[2] = 0.550f; m_angularVelocityCurve[3] = 0.950f;
			parameterChanged = true;
		}

	}

	ImGui::Spacing();

	return parameterChanged;
}

void EmitterRotation::updateAlivesRotationFixed(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<std::pair<float, unsigned int>>& aliveParticles, float deltaTime)
{

	for (auto& aliveParticle : aliveParticles)
	{
		unsigned int poolIndex = aliveParticle.second;

		float rotationVelocity = particlePool[poolIndex].rotationVelocity;
		float lastRotation = particlePool[poolIndex].rotationZ;

		particlePool[poolIndex].rotationZ = getNormalizedAngle(lastRotation + deltaTime * rotationVelocity);
	}
}

void EmitterRotation::updateAlivesRotationWithCurve(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<std::pair<float, unsigned int>>& aliveParticles, float deltaTime, float startLifetime)
{
	for (auto& aliveParticle : aliveParticles)
	{
		unsigned int poolIndex = aliveParticle.second;

		float rotationVelocity = particlePool[poolIndex].rotationVelocity;
		float lastRotation = particlePool[poolIndex].rotationZ;

		particlePool[poolIndex].rotationZ = getNormalizedAngle(lastRotation + deltaTime * rotationVelocity);

		// Get new rotation velocity from curve (based on current lifetime)

		float scale = 1.f - particlePool[poolIndex].lifeTime/ startLifetime; // to start with 0
		float bezierScale = ImGui::BezierValue(scale, m_angularVelocityCurve);

		particlePool[poolIndex].rotationVelocity = particlePool[poolIndex].flippedRotation ? 
			-(m_angularVelocity + (m_angularVelocity2 - m_angularVelocity) * bezierScale) :
			  m_angularVelocity + (m_angularVelocity2 - m_angularVelocity) * bezierScale;
	}
}

void EmitterRotation::setNewParticlesVelocityFixed(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	if (m_startRotationType == ParameterType::CONSTANT) // we just assign the fixed start rotation value, alongside the velocity 
	{

		for (auto& particleIndex : newParticles)
		{
			particlePool[particleIndex].rotationZ = m_startRotation;

			if (uniform_rand() + 0.001f <= m_flipRotationLikelihood) // + 0.001f so that m_flipRotationLikelihood == 0 can be used as no flip case  
			{
				particlePool[particleIndex].rotationVelocity = -m_angularVelocity;
				particlePool[particleIndex].flippedRotation = true;
			}
			else
			{
				particlePool[particleIndex].rotationVelocity = m_angularVelocity;
				particlePool[particleIndex].flippedRotation = false;
			}
		}

	}
	else 
	{ // => generate random start rotation

		for (auto& particleIndex : newParticles)
		{
			float scale = uniform_rand();
			float randomStartRotation = m_startRotation + (m_startRotation2 - m_startRotation) * scale;
			particlePool[particleIndex].rotationZ = randomStartRotation;

			if (uniform_rand() + 0.001f <= m_flipRotationLikelihood) // + 0.001f so that m_flipRotationLikelihood == 0 can be used as no flip case  
			{
				particlePool[particleIndex].rotationVelocity = -m_angularVelocity;
				particlePool[particleIndex].flippedRotation = true;
			}
			else
			{
				particlePool[particleIndex].rotationVelocity = m_angularVelocity;
				particlePool[particleIndex].flippedRotation = false;
			}
		}
	}
}

void EmitterRotation::setNewParticlesVelocityWithRange(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	if (m_startRotationType == ParameterType::CONSTANT) // we just assign the fixed start rotation value, alongside the velocity
	{
		for (auto& particleIndex : newParticles)
		{
			particlePool[particleIndex].rotationZ = m_startRotation;

			float scale = uniform_rand();
			float randomAngularVelocity = m_angularVelocity + (m_angularVelocity2 - m_angularVelocity) * scale;

			if (uniform_rand() + 0.001f <= m_flipRotationLikelihood) // + 0.001f so that m_flipRotationLikelihood == 0 can be used as no flip case  
			{
				particlePool[particleIndex].rotationVelocity = -randomAngularVelocity;
				particlePool[particleIndex].flippedRotation = true;
			}
			else
			{
				particlePool[particleIndex].rotationVelocity = randomAngularVelocity;
				particlePool[particleIndex].flippedRotation = false;
			}
		}

	}
	else 
	{ // => generate random start rotation

		for (auto& particleIndex : newParticles)
		{
			float scale = uniform_rand();
			float randomStartRotation = m_startRotation + (m_startRotation2 - m_startRotation) * scale;
			particlePool[particleIndex].rotationZ = randomStartRotation;

			scale = uniform_rand(); // we reuse for angular velocity
			float randomAngularVelocity = m_angularVelocity + (m_angularVelocity2 - m_angularVelocity) * scale;

			if (uniform_rand() + 0.001f <= m_flipRotationLikelihood) // + 0.001f so that m_flipRotationLikelihood == 0 can be used as no flip case  
			{
				particlePool[particleIndex].rotationVelocity = -randomAngularVelocity;
				particlePool[particleIndex].flippedRotation = true;
			}
			else
			{
				particlePool[particleIndex].rotationVelocity = randomAngularVelocity;
				particlePool[particleIndex].flippedRotation = false;
			}
		}
	}
}

inline float EmitterRotation::getNormalizedAngle(float angle)
{
	while (angle >= XM_2PI) angle -= XM_2PI;
	while (angle < 0) angle += XM_2PI;

	return angle;
}
