#include "Globals.h"
#include "EmitterAnimation.h"

#include "Application.h"

#include "ModuleParticleSystem.h"
#include "ParticleSystemComponent.h"
#include "EmitterInstance.h"

void EmitterAnimation::update(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();

		// Dealing with already existing particles //

		float deltaTime = particleData->getParticleSystemComponent()->deltaTime();

		for (auto& aliveParticle : aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float newFrame = particlePool[poolIndex].textureFrame + deltaTime * m_fps;
			
			if (newFrame >= m_totalFrames) 
			{ 
				particlePool[poolIndex].textureFrame = newFrame - m_totalFrames;  // could happen that newFrame >= 2*m_totalFrames ? (if so, we need mod operation)
			}
			else particlePool[poolIndex].textureFrame = newFrame;
		}
	}

	// Initialization for new ones //

	if (m_startFrameType == ParameterType::CONSTANT) 
	{
		setNewParticlesFrameConstant(particlePool, particleData->getNewParticles());
	}
	else 
	{
		setNewParticlesFrameRandom(particlePool, particleData->getNewParticles());
	}

}

Vector2 EmitterAnimation::getUVScale() const
{
	return Vector2(1.f / static_cast<float>(m_columns), 1.f / static_cast<float>(m_rows));
}

Vector2 EmitterAnimation::getUVOffset(int particleIndex) const
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();

	const int index = static_cast<int>(std::clamp(particlePool[particleIndex].textureFrame, 0.f, m_totalFrames - 1));
	const int col = (m_columns > 0) ? (index % m_columns) : 0;
	const int row = (m_columns > 0) ? (index / m_columns) : 0;

	const float invCols = 1.0f / float(std::max(1, m_columns));
	const float invRows = 1.0f / float(std::max(1, m_rows));

	Vector2 uvOffset = { static_cast<float>(col) * invCols, static_cast<float>(row) * invRows }; // we may want to add an offset to this

	return uvOffset;
}

bool EmitterAnimation::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Animation")) {

		if (ImGui::DragInt("Tile rows", &m_rows, 1.f, 1))
		{
			m_totalFrames = static_cast<float>(m_rows * m_columns);
			parameterChanged = true;
		}

		if (ImGui::DragInt("Tile columns", &m_columns, 1.f, 1))
		{
			m_totalFrames = static_cast<float>(m_rows * m_columns);
			parameterChanged = true;
		}

		ImGui::Spacing();

		parameterChanged |= ImGui::DragFloat("FPS", &m_fps, 0.1f, 0.f);

		ImGui::Spacing();

		parameterChanged |= drawStartFrameUI();
	}

	return parameterChanged;
}

rapidjson::Value EmitterAnimation::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::ANIMATION), domTree.GetAllocator());

	moduleInfo.AddMember("TileRows", m_rows, domTree.GetAllocator());
	moduleInfo.AddMember("TileColumns", m_columns, domTree.GetAllocator());

	moduleInfo.AddMember("FPS", m_fps, domTree.GetAllocator());

	moduleInfo.AddMember("StartFrameType", unsigned int(m_startFrameType), domTree.GetAllocator());
	moduleInfo.AddMember("StartFrame", m_startFrame, domTree.GetAllocator());
	if (m_startFrameType == ParameterType::RANDOM_BETWEEN_TWO) 
	{
		moduleInfo.AddMember("StartFrame2", m_startFrame2, domTree.GetAllocator());
	}

	return moduleInfo;
}

bool EmitterAnimation::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("TileRows")) {
		m_rows = moduleInfo["TileRows"].GetInt();
	}
	if (moduleInfo.HasMember("TileColumns")) {
		m_columns = moduleInfo["TileColumns"].GetInt();
	}

	if (moduleInfo.HasMember("FPS")) {
		m_fps = moduleInfo["FPS"].GetFloat();
	}

	if (moduleInfo.HasMember("StartFrameType")) 
	{
		unsigned int frameTypeUInt = moduleInfo["StartFrameType"].GetUint();
		ParameterType frameType = static_cast<ParameterType>(frameTypeUInt);

		switch (frameType) {

		case ParameterType::CONSTANT:

			m_startFrameType = ParameterType::CONSTANT;

			if (moduleInfo.HasMember("StartFrame")) 
			{
				m_startFrame = moduleInfo["StartFrame"].GetFloat();
			}
			
			break;

		case ParameterType::RANDOM_BETWEEN_TWO:

			m_startFrameType = ParameterType::RANDOM_BETWEEN_TWO;

			if (moduleInfo.HasMember("StartFrame"))
			{
				m_startFrame = moduleInfo["StartFrame"].GetFloat();
			}

			if (moduleInfo.HasMember("StartFrame2"))
			{
				m_startFrame2 = moduleInfo["StartFrame2"].GetFloat();
			}

		}
	
	}

	m_totalFrames = static_cast<float>(m_rows * m_columns);

	return true;
}

bool EmitterAnimation::drawStartFrameUI()
{
	bool parameterChanged = false;

	// Type selection combo (COULD BE REPLACED WITH SOMETHING SMALLER?)
	{
		int parameterType = static_cast<int>(m_startFrameType);
		if (ImGui::Combo("Starting frame type", &parameterType, "Constant\0Random value between two\0", static_cast<int>(ParameterType::TOTAL_TYPES))) // (curve only if needed)
		{
			m_startFrameType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}
	}

	switch (m_startFrameType) {

	case ParameterType::CONSTANT:

	{
		parameterChanged |= ImGui::DragFloat("Starting frame", &m_startFrame, 0.1f, 0.f, 0.999);
	}
	
	break;

	case ParameterType::RANDOM_BETWEEN_TWO:

	{
		parameterChanged |= ImGui::DragFloat("Starting frame", &m_startFrame, 0.1f, 0.f, 0.999);
		parameterChanged |= ImGui::DragFloat("Starting frame 2", &m_startFrame2, 0.1f, 0.f, 0.999);
	}

	}

	return parameterChanged;
}

void EmitterAnimation::setNewParticlesFrameConstant(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	float frame = m_startFrame * m_totalFrames;

	for (auto& particleIndex : newParticles)
	{
		particlePool[particleIndex].textureFrame = frame;
	}
}

void EmitterAnimation::setNewParticlesFrameRandom(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	for (auto& particleIndex : newParticles)
	{
		float scale = uniform_rand();
		float randomFrameNorm = (1.f - scale) * m_startFrame + scale * m_startFrame2; // value should be [0, 1)

		particlePool[particleIndex].textureFrame = randomFrameNorm * m_totalFrames;
	}
}
