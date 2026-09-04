#include "pch.h"
#include "AelorinLavaController.h"
#include <algorithm>

IMPLEMENT_SCRIPT_FIELDS(AelorinLavaController,
	SERIALIZED_COMPONENT_REF(m_LavaTransform, "LavaTransform", ComponentType::TRANSFORM)
)

AelorinLavaController::AelorinLavaController(GameObject* owner) : Script(owner)
{
}

void AelorinLavaController::StartLavaRise(float height, float duration)
{
	m_startHeight = ComponentAPI::getOwner(m_LavaTransform.getReferencedComponent())->GetTransform()->getPosition().y;
	m_targetHeight = height;
	m_duration = duration > 0.0f? duration:0.01f;
	m_elapsedTime = 0.0f;
	m_animating = true;
}

void AelorinLavaController::StartLavaFall(float height, float duration)
{
	StartLavaRise(height, duration);
}

void AelorinLavaController::Update()
{
	if (!m_animating || !m_LavaTransform.getReferencedComponent())
	{
		return;
	}

	m_elapsedTime += Time::getDeltaTime();
	const float t = std::min<float>(m_elapsedTime / m_duration, 1.0f);
	const float eased = smoothStep(t);

	Vector3 pos = ComponentAPI::getOwner(m_LavaTransform.getReferencedComponent())->GetTransform()->getPosition();
	pos.y = m_startHeight + (m_targetHeight - m_startHeight) * eased;
	TransformAPI::setPosition(m_LavaTransform.getReferencedComponent(), pos);

	if (t >= 1.0f)
	{
		m_animating = false;
	}
}

float AelorinLavaController::smoothStep(float t) const
{
	return t * t * (3.0f - 2.0f * t);
}

IMPLEMENT_SCRIPT(AelorinLavaController)