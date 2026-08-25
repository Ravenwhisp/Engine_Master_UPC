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
    newComponent->m_revealStrength = m_revealStrength;
    newComponent->m_bubbleScale = m_bubbleScale;
    newComponent->m_bubbleSoftness = m_bubbleSoftness;
    newComponent->m_occluderOpacity = m_occluderOpacity;

    return newComponent;
}

void OcclusionTargetComponent::drawUi()
{
    if (ImGui::CollapsingHeader("Occlusion Reveal Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Reveal Strength", &m_revealStrength, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Bubble Scale", &m_bubbleScale, 0.01f, 0.5f, 3.0f, "%.2f");
        ImGui::DragFloat("Bubble Softness", &m_bubbleSoftness, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Occluder Opacity", &m_occluderOpacity, 0.01f, 0.0f, 1.0f, "%.2f");
    }
}

void OcclusionTargetComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serialize(m_revealStrength, "RevealStrength");
    archive.serialize(m_bubbleScale, "BubbleScale");
    archive.serialize(m_bubbleSoftness, "BubbleSoftness");
    archive.serialize(m_occluderOpacity, "OccluderOpacity");

    m_revealStrength = std::clamp(m_revealStrength, 0.0f, 1.0f);
    m_bubbleScale = std::clamp(m_bubbleScale, 0.5f, 3.0f);
    m_bubbleSoftness = std::clamp(m_bubbleSoftness, 0.0f, 1.0f);
    m_occluderOpacity = std::clamp(m_occluderOpacity, 0.0f, 1.0f);
}