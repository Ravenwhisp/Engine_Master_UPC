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

	for (auto& particleIndex : particleData->getNewParticles())
	{
		particlePool[particleIndex].textureFrame = 0.f; // until we have other options
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

	if (ImGui::CollapsingHeader("Render & Animation")) {

		parameterChanged = ImGui::DragInt("Render layer", &m_layer, 1.f);
		
		ImGui::Spacing(); ImGui::Spacing();

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
	}

	return parameterChanged;
}

rapidjson::Value EmitterAnimation::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::ANIMATION), domTree.GetAllocator());

	moduleInfo.AddMember("RenderLayer", m_layer, domTree.GetAllocator());

	moduleInfo.AddMember("TileRows", m_rows, domTree.GetAllocator());
	moduleInfo.AddMember("TileColumns", m_columns, domTree.GetAllocator());

	moduleInfo.AddMember("FPS", m_fps, domTree.GetAllocator());

	return moduleInfo;
}

bool EmitterAnimation::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("RenderLayer")) {
		m_layer = moduleInfo["RenderLayer"].GetInt();
	}

	if (moduleInfo.HasMember("TileRows")) {
		m_rows = moduleInfo["TileRows"].GetInt();
	}
	if (moduleInfo.HasMember("TileColumns")) {
		m_columns = moduleInfo["TileColumns"].GetInt();
	}

	if (moduleInfo.HasMember("FPS")) {
		m_fps = moduleInfo["FPS"].GetFloat();
	}

	m_totalFrames = static_cast<float>(m_rows * m_columns);

	return true;
}
