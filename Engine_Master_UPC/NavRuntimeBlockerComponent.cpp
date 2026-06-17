#include "Globals.h"
#include "NavRuntimeBlockerComponent.h"
#include "IArchive.h"

#include "Application.h"
#include "ModuleNavigation.h"
#include "GameObject.h"
#include "Transform.h"

NavRuntimeBlockerComponent::NavRuntimeBlockerComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAV_RUNTIME_BLOCKER, owner)
{
}

std::unique_ptr<Component> NavRuntimeBlockerComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<NavRuntimeBlockerComponent> newComponent = std::make_unique<NavRuntimeBlockerComponent>(m_uuid, newOwner);

	newComponent->m_halfExtents = m_halfExtents;
	newComponent->m_blocked = m_blocked;

	return newComponent;
}

void NavRuntimeBlockerComponent::drawUi()
{
	ImGui::SeparatorText("Nav Runtime Blocker");

	ImGui::DragFloat3("Half Extents", &m_halfExtents.x, 0.1f, 0.01f);

	bool blocked = m_blocked;
	if (ImGui::Checkbox("Blocked", &blocked))
	{
		setBlocked(blocked);
	}
}

void NavRuntimeBlockerComponent::onTransformChange()
{
}

void NavRuntimeBlockerComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);
	archive.serialize(m_halfExtents, "HalfExtents");
	archive.serialize(m_blocked, "Blocked");
}

void NavRuntimeBlockerComponent::debugDraw()
{
	Transform* transform = getOwner()->GetTransform();
	if (!transform)
	{
		return;
	}

	const Vector3 center = transform->getGlobalMatrix().Translation();

	const float* color = m_blocked ? dd::colors::Red : dd::colors::Green;

	dd::aabb(ddConvert(center - m_halfExtents), ddConvert(center + m_halfExtents), color);
}

void NavRuntimeBlockerComponent::setBlocked(bool blocked)
{
	m_blocked = blocked;
}