#include "Globals.h"
#include "OcclusionTargetComponent.h"

#include "JsonArchive.h"

#include <algorithm>

OcclusionTargetComponent::OcclusionTargetComponent(UID id, GameObject* owner) : Component(id, ComponentType::OCCLUSION_TARGET, owner)
{
}

std::unique_ptr<Component> OcclusionTargetComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<OcclusionTargetComponent> newComponent = std::make_unique<OcclusionTargetComponent>(m_uuid, newOwner);
    newComponent->setActive(isActive());
    newComponent->m_bubbleScale = m_bubbleScale;
    newComponent->m_bubbleSoftness = m_bubbleSoftness;
    return newComponent;
}

void OcclusionTargetComponent::drawUi()
{
    if (ImGui::CollapsingHeader("Dynamic Transparency", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Falloff Scale", &m_bubbleScale, 0.01f, 1.0f, 3.0f, "%.2f");
        ImGui::DragFloat("Falloff Softness", &m_bubbleSoftness, 0.01f, 0.0f, 1.0f, "%.2f");
    }
}

void OcclusionTargetComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serialize(m_bubbleScale, "BubbleScale");
    archive.serialize(m_bubbleSoftness, "BubbleSoftness");

    m_bubbleScale = std::clamp(m_bubbleScale, 1.0f, 3.0f);
    m_bubbleSoftness = std::clamp(m_bubbleSoftness, 0.0f, 1.0f);
}