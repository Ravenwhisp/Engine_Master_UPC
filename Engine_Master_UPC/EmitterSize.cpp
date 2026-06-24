#include "Globals.h"
#include "EmitterSize.h"

//#include "GameObject.h"
//#include "Transform.h"
#include "Application.h"
#include "imgui_bezier.h"

#include "ModuleParticleSystem.h"
//#include "ParticleSystemComponent.h"
#include "EmitterInstance.h"
#include "ParticleEmitter.h"
#include "EmitterLifetime.h"

void EmitterSize::update(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();

	//Vector3 parentScale = particleData->getParticleSystemComponent()->getOwner()->GetTransform()->getScale();
	//Vector2 parentScale2D = Vector2(parentScale.x, parentScale.y); // because for now we are only going to use these

	if (m_changeSizeOverTime)
	{
		std::vector<std::pair<float, unsigned int>>& aliveParticles = particleData->getAliveParticles();

		// Dealing with already existing particles //

		float startLifetime = particleData->getParticleEmitter()->getLifetimeModule()->getStartLifetime();

		for (auto& aliveParticle : aliveParticles)
		{
			unsigned int poolIndex = aliveParticle.second;

			float scale = particlePool[poolIndex].lifeTime / startLifetime;

			particlePool[poolIndex].scale = Vector2::Lerp(particlePool[poolIndex].endScale, particlePool[poolIndex].startScale, scale); // We need to use Bezier curves instead of this
		}
	}

	// Initialization for new ones //

	//Vector2 finalInitialScale = parentScale2D * m_startScale;
	switch (m_startScaleType) {

	case ParameterType::CONSTANT:

		setNewParticlesScaleConstant(particlePool, particleData->getNewParticles());
		break;
	
	case ParameterType::RANDOM_BETWEEN_TWO:

		setNewParticlesScaleRandom(particlePool, particleData->getNewParticles());
		break;
	}
}

bool EmitterSize::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Size"))
	{

		// We will have at least one scale value; 2 values to interpolate if we allow it

		parameterChanged = drawStartScaleUI();

		parameterChanged |= ImGui::Checkbox("Change size over time##Size", &m_changeSizeOverTime);
		if (!m_changeSizeOverTime) return parameterChanged;

		parameterChanged |= drawEndScaleUI();
	}

	return parameterChanged;
}

void EmitterSize::serialize(IArchive& archive)
{
	ParticleModule::serialize(archive);

	archive.serializeStringEnum(m_startScaleType, "StartScaleType", ParameterTypeToString, StringToParameterType);

	{
		DirectX::SimpleMath::Vector3 v(m_startScale.x, m_startScale.y, 0.0f);
		archive.serialize(v, "StartScale");
		if (archive.mode() == ArchiveMode::Input)
		{
			m_startScale.x = v.x;
			m_startScale.y = v.y;
		}
	}

	if (m_startScaleType != ParameterType::CONSTANT)
	{
		{
			DirectX::SimpleMath::Vector3 v(m_startScale2.x, m_startScale2.y, 0.0f);
			archive.serialize(v, "StartScale2");
			if (archive.mode() == ArchiveMode::Input)
			{
				m_startScale2.x = v.x;
				m_startScale2.y = v.y;
			}
		}
	}

	archive.serialize(m_changeSizeOverTime, "ChangeSizeOverTime");

	if (m_changeSizeOverTime)
	{
		archive.serializeStringEnum(m_endScaleType, "EndScaleType", ParameterTypeToString, StringToParameterType);

		{
			DirectX::SimpleMath::Vector3 v(m_endScale.x, m_endScale.y, 0.0f);
			archive.serialize(v, "EndScale");
			if (archive.mode() == ArchiveMode::Input)
			{
				m_endScale.x = v.x;
				m_endScale.y = v.y;
			}
		}

		if (m_endScaleType != ParameterType::CONSTANT)
		{
			DirectX::SimpleMath::Vector3 v(m_endScale2.x, m_endScale2.y, 0.0f);
			archive.serialize(v, "EndScale2");
			if (archive.mode() == ArchiveMode::Input)
			{
				m_endScale2.x = v.x;
				m_endScale2.y = v.y;
			}
		}
	}
}

bool EmitterSize::drawStartScaleUI()
{
	bool parameterChanged = false;

	// Type selection combo (COULD BE REPLACED WITH SOMETHING SMALLER?)
	{
		int parameterType = static_cast<int>(m_startScaleType);
		if (ImGui::Combo("Starting scale type##Size", &parameterType, "Constant\0Random value between two\0", static_cast<int>(ParameterType::TOTAL_TYPES))) // (will add curve later)
		{
			m_startScaleType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}
	}

	switch (m_startScaleType) {

	case ParameterType::CONSTANT:

		{
			float scale[2] = { m_startScale.x, m_startScale.y };
			if (ImGui::DragFloat2("Starting scale##Size", scale, 0.1f, 0.f))
			{
				Vector2 newScale = Vector2(scale[0], scale[1]);
				m_startScale = newScale;
				parameterChanged = true;
			}
		}
		break;


	case ParameterType::RANDOM_BETWEEN_TWO:

		{
			float scale[2] = { m_startScale.x, m_startScale.y };
			if (ImGui::DragFloat2("Starting scale 1##Size", scale, 0.1f, 0.f))
			{
				Vector2 newScale = Vector2(scale[0], scale[1]);
				m_startScale = newScale;
				parameterChanged = true;
			}

			scale[0] = m_startScale2.x; scale[1] = m_startScale2.y;
			if (ImGui::DragFloat2("Starting scale 2##Size", scale, 0.1f, 0.f))
			{
				Vector2 newScale = Vector2(scale[0], scale[1]);
				m_startScale2 = newScale;
				parameterChanged = true;
			}
		}
		break;


	case ParameterType::CURVE:

		// 1. Range of values (for that, we use the 2 constants)
		{
			float scale[2] = { m_startScale.x, m_startScale.y };
			if (ImGui::DragFloat2("Starting scale 1##Size", scale, 0.1f, 0.f))
			{
				Vector2 newScale = Vector2(scale[0], scale[1]);
				m_startScale = newScale;
				parameterChanged = true;
			}

			scale[0] = m_startScale2.x; scale[1] = m_startScale2.y;
			if (ImGui::DragFloat2("Starting scale 2##Size", scale, 0.1f, 0.f))
			{
				Vector2 newScale = Vector2(scale[0], scale[1]);
				m_startScale2 = newScale;
				parameterChanged = true;
			}
		}

		// 2. Curve (between 0 and 1)

		if (ImGui::Bezier("Curve##Size", m_startScaleCurve))
		{
			parameterChanged = true;
		}

		// We add some buttons to quickly change to predefined setups (if we have these in multiple modules, maybe we want them declared in a single file, instead of duplicating)
		if (ImGui::Button("Linear##Size"))
		{
			m_startScaleCurve[0] = 0.000f; m_startScaleCurve[1] = 0.000f; m_startScaleCurve[2] = 1.000f; m_startScaleCurve[3] = 1.000f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseIn##Size"))
		{
			m_startScaleCurve[0] = 0.470f; m_startScaleCurve[1] = 0.000f; m_startScaleCurve[2] = 0.745f; m_startScaleCurve[3] = 0.715f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseOut##Size"))
		{
			m_startScaleCurve[0] = 0.390f; m_startScaleCurve[1] = 0.575f; m_startScaleCurve[2] = 0.565f; m_startScaleCurve[3] = 1.000f;
			parameterChanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("EaseInOut##Size"))
		{
			m_startScaleCurve[0] = 0.445f; m_startScaleCurve[1] = 0.050f; m_startScaleCurve[2] = 0.550f; m_startScaleCurve[3] = 0.950f;
			parameterChanged = true;
		}

	}

	ImGui::Spacing();

	return parameterChanged;
}

bool EmitterSize::drawEndScaleUI()
{
	bool parameterChanged = false;

	// Type selection combo (COULD BE REPLACED WITH SOMETHING SMALLER?)
	{
		int parameterType = static_cast<int>(m_endScaleType);
		if (ImGui::Combo("End scale type##Size", &parameterType, "Constant\0Random value between two\0", static_cast<int>(ParameterType::TOTAL_TYPES))) // (will add curve later)
		{
			m_endScaleType = static_cast<ParameterType>(parameterType);
			parameterChanged = true;
		}
	}

	switch (m_endScaleType) {

	case ParameterType::CONSTANT:

	{
		float scale[2] = { m_endScale.x, m_endScale.y };
		if (ImGui::DragFloat2("End scale##Size", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale = newScale;
			parameterChanged = true;
		}
	}
	break;


	case ParameterType::RANDOM_BETWEEN_TWO:

	{
		float scale[2] = { m_endScale.x, m_endScale.y };
		if (ImGui::DragFloat2("End scale 1##Size", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale = newScale;
			parameterChanged = true;
		}

		scale[0] = m_endScale2.x; scale[1] = m_endScale2.y;
		if (ImGui::DragFloat2("End scale 2##Size", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale2 = newScale;
			parameterChanged = true;
		}
	}
	break;


	case ParameterType::CURVE:

		// 1. Range of values (for that, we use the 2 constants)
	{
		float scale[2] = { m_endScale.x, m_endScale.y };
		if (ImGui::DragFloat2("End scale 1##Size", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale = newScale;
			parameterChanged = true;
		}

		scale[0] = m_endScale2.x; scale[1] = m_endScale2.y;
		if (ImGui::DragFloat2("End scale 2##Size", scale, 0.1f, 0.f))
		{
			Vector2 newScale = Vector2(scale[0], scale[1]);
			m_endScale2 = newScale;
			parameterChanged = true;
		}
	}

	// 2. Curve (between 0 and 1)

	if (ImGui::Bezier("Curve##Size", m_endScaleCurve))
	{
		parameterChanged = true;
	}

	// We add some buttons to quickly change to predefined setups (if we have these in multiple modules, maybe we want them declared in a single file, instead of duplicating)
	if (ImGui::Button("Linear##Size"))
	{
		m_endScaleCurve[0] = 0.000f; m_endScaleCurve[1] = 0.000f; m_endScaleCurve[2] = 1.000f; m_endScaleCurve[3] = 1.000f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseIn##Size"))
	{
		m_endScaleCurve[0] = 0.470f; m_endScaleCurve[1] = 0.000f; m_endScaleCurve[2] = 0.745f; m_endScaleCurve[3] = 0.715f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseOut##Size"))
	{
		m_endScaleCurve[0] = 0.390f; m_endScaleCurve[1] = 0.575f; m_endScaleCurve[2] = 0.565f; m_endScaleCurve[3] = 1.000f;
		parameterChanged = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("EaseInOut##Size"))
	{
		m_endScaleCurve[0] = 0.445f; m_endScaleCurve[1] = 0.050f; m_endScaleCurve[2] = 0.550f; m_endScaleCurve[3] = 0.950f;
		parameterChanged = true;
	}

	}

	return parameterChanged;
}

void EmitterSize::setNewParticlesScaleConstant(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	if (!m_changeSizeOverTime) { // just assign an initial scale, don't touch startScale, endScale

		for (auto& particleIndex : newParticles)
		{
			particlePool[particleIndex].scale = m_startScale;
		}

		return;
	}


	// We will have to assign a future end scale for the particles based on the config
	switch (m_endScaleType) {

	case ParameterType::CONSTANT:

		for (auto& particleIndex : newParticles)
		{
			particlePool[particleIndex].scale = m_startScale;
			particlePool[particleIndex].startScale = m_startScale; // for later
			particlePool[particleIndex].endScale = m_endScale;	   //
		}
		break;
	
	case ParameterType::RANDOM_BETWEEN_TWO:

		for (auto& particleIndex : newParticles)
		{
			particlePool[particleIndex].scale = m_startScale;
			particlePool[particleIndex].startScale = m_startScale; // for later

			float scale = uniform_rand();
			Vector2 randomEndScale = Vector2::Lerp(m_endScale, m_endScale2, scale);
			particlePool[particleIndex].endScale = randomEndScale; // for later as well
		}
		break;
	
	}
}

void EmitterSize::setNewParticlesScaleRandom(std::array<Particle, MAX_PARTICLES>& particlePool, const std::vector<unsigned int>& newParticles)
{
	if (!m_changeSizeOverTime) { // just assign an initial scale, don't touch startScale, endScale

		for (auto& particleIndex : newParticles)
		{
			float scale = uniform_rand();
			Vector2 randomStartScale = Vector2::Lerp(m_startScale, m_startScale2, scale);
			particlePool[particleIndex].scale = randomStartScale;
		}

		return;
	}


	// We will have to assign a future end scale for the particles based on the config
	switch (m_endScaleType) {

	case ParameterType::CONSTANT:

		for (auto& particleIndex : newParticles)
		{
			float scale = uniform_rand();
			Vector2 randomStartScale = Vector2::Lerp(m_startScale, m_startScale2, scale);

			particlePool[particleIndex].scale = randomStartScale;
			particlePool[particleIndex].startScale = randomStartScale; // for later
			particlePool[particleIndex].endScale = m_endScale;		   //
		}
		break;

	case ParameterType::RANDOM_BETWEEN_TWO:

		for (auto& particleIndex : newParticles)
		{
			float scale = uniform_rand();
			Vector2 randomStartScale = Vector2::Lerp(m_startScale, m_startScale2, scale);

			particlePool[particleIndex].scale = randomStartScale;
			particlePool[particleIndex].startScale = randomStartScale; // for later

			// now the same process, but for endScale
			scale = uniform_rand();
			Vector2 randomEndScale = Vector2::Lerp(m_endScale, m_endScale2, scale);
			particlePool[particleIndex].endScale = randomEndScale; // for later as well
		}
		break;

	}
}
