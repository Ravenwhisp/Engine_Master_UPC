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
    newComponent->m_bodyHeight = m_bodyHeight;
    newComponent->m_anchorHeight = m_anchorHeight;
    newComponent->m_occlusionMargin = m_occlusionMargin;
    return newComponent;
}

void OcclusionTargetComponent::drawUi()
{
    if (ImGui::CollapsingHeader("Dynamic Transparency", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Falloff Scale", &m_bubbleScale, 0.01f, 1.0f, 3.0f, "%.2f");
        ImGui::DragFloat("Falloff Softness", &m_bubbleSoftness, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Body Height (0 = auto)", &m_bodyHeight, 0.01f, 0.0f, 10000.0f);
        ImGui::DragFloat("Torso Height Fraction", &m_anchorHeight, 0.01f, 0.2f, 0.8f);
        ImGui::DragFloat("Occlusion Margin Fraction", &m_occlusionMargin, 0.001f, 0.001f, 0.25f);
        ImGui::TextWrapped("Place this component on the movement root at the feet, not an animated bone. Auto height uses the first skinned body mesh; set Body Height explicitly for unusual hierarchies.");
    }
}

void OcclusionTargetComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serialize(m_bubbleScale, "BubbleScale");
    archive.serialize(m_bubbleSoftness, "BubbleSoftness");
    archive.serialize(m_bodyHeight, "OcclusionBodyHeight");
    archive.serialize(m_anchorHeight, "OcclusionAnchorHeight");
    archive.serialize(m_occlusionMargin, "OcclusionMargin");

    m_bubbleScale = std::clamp(m_bubbleScale, 1.0f, 3.0f);
    m_bubbleSoftness = std::clamp(m_bubbleSoftness, 0.0f, 1.0f);
    m_bodyHeight = std::max(m_bodyHeight, 0.0f);
    m_anchorHeight = std::clamp(m_anchorHeight, 0.2f, 0.8f);
    m_occlusionMargin = std::clamp(m_occlusionMargin, 0.001f, 0.25f);
}
