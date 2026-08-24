#include "Globals.h"
#include "FlowMapComponent.h"

#include "Application.h"
#include "JsonArchive.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"
#include "ModuleTime.h"
#include "Texture.h"

#include <cmath>

FlowMapComponent::FlowMapComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::FLOW_MAP, owner)
{
}

std::unique_ptr<Component> FlowMapComponent::clone(GameObject* newOwner) const
{
    auto result = std::make_unique<FlowMapComponent>(m_uuid, newOwner);
    result->m_data = m_data;
    result->m_textureAssetId = m_textureAssetId;
    result->m_texture = m_texture;
    result->m_textureAsset = m_textureAsset;
    result->m_phase = m_phase;
    return result;
}

void FlowMapComponent::update()
{
    if (!m_data.enabled || m_data.speed == 0.0f)
        return;

    const float dt = app->getModuleTime()->deltaTime();
    if (getTechnique() == FlowMapTechnique::FLOW_MAP_WATER)
    {
        m_phase += m_data.speed * dt;
        m_phase -= std::floor(m_phase);
    }
    else
    {
        m_data.offset += m_data.direction * (m_data.speed * dt);
        m_data.offset.x -= std::floor(m_data.offset.x);
        m_data.offset.y -= std::floor(m_data.offset.y);
    }

#ifdef _DEBUG
    const uint32_t frame = app->getModuleTime()->frameCount();
    if ((frame % 60u) == 0u)
    {
        DEBUG_LOG("[FlowMap] component=%llu offset=(%.4f, %.4f) speed=%.3f source=%u\n",
            static_cast<unsigned long long>(m_uuid),
            m_data.offset.x, m_data.offset.y, m_data.speed, m_data.source);
    }
#endif
}

void FlowMapComponent::drawUi()
{
    if (!ImGui::CollapsingHeader("Flow Map Settings", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    int technique = static_cast<int>(m_data.technique);
    const char* techniques[] = { "Directional Scroll", "Flow Map Water" };
    if (ImGui::Combo("Technique", &technique, techniques, 2))
        m_data.technique = static_cast<uint32_t>(technique);

    if (getTechnique() == FlowMapTechnique::FLOW_MAP_WATER)
    {
        int source = static_cast<int>(m_data.source);
        const char* sources[] = { "Direction", "Texture" };
        if (ImGui::Combo("Source", &source, sources, 2))
            m_data.source = static_cast<uint32_t>(source);
    }

    ImGui::DragFloat2("Direction", &m_data.direction.x, 0.01f, -1.0f, 1.0f);
    ImGui::DragFloat2("Tiling", &m_data.tiling.x, 0.01f, 0.001f, 100.0f);
    ImGui::DragFloat("Speed", &m_data.speed, 0.01f, -100.0f, 100.0f);
    ImGui::DragFloat("Strength", &m_data.strength, 0.01f, 0.0f, 10.0f);
    bool enabled = m_data.enabled != 0;
    if (ImGui::Checkbox("Enabled", &enabled))
        m_data.enabled = enabled ? 1u : 0u;

    if (getSource() == FlowMapSource::TEXTURE)
    {
        ImGui::Button("Drop Flow Texture Here");
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                UID* uid = static_cast<UID*>(payload->Data);
                AssetId* reference = app->getModuleAssets()->findReference(*uid);
                if (reference)
                    setTextureAssetId(*reference);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        ImGui::Text("Loaded: %s", m_texture ? "YES" : "NO");
    }
}

void FlowMapComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);
    archive.serialize(m_data.direction, "Direction");
    archive.serialize(m_data.tiling, "Tiling");
    archive.serialize(m_data.speed, "Speed");
    archive.serialize(m_data.strength, "Strength");
    archive.serialize(m_data.offset, "Offset");
    archive.serialize(m_data.source, "Source");
    archive.serialize(m_data.enabled, "Enabled");
    archive.serialize(m_data.technique, "Technique");

    if (archive.mode() == ArchiveMode::Input)
    {
        if (m_data.source > static_cast<uint32_t>(FlowMapSource::TEXTURE))
            m_data.source = static_cast<uint32_t>(FlowMapSource::DIRECTION);
        if (m_data.technique > static_cast<uint32_t>(FlowMapTechnique::FLOW_MAP_WATER))
            m_data.technique = static_cast<uint32_t>(FlowMapTechnique::DIRECTIONAL_SCROLL);
    }

    archive.beginObject("TextureAssetId");
    m_textureAssetId.serialize(archive);
    archive.endObject();

    if (archive.mode() == ArchiveMode::Input)
        loadTexture();
}

void FlowMapComponent::setTextureAssetId(const AssetId& id)
{
    m_textureAssetId = id;
    loadTexture();
}

void FlowMapComponent::loadTexture()
{
    m_texture.reset();
    m_textureAsset.reset();
    if (!m_textureAssetId.isValid())
        return;

    m_textureAsset = app->getModuleAssets()->load<TextureAsset>(m_textureAssetId);
    if (m_textureAsset)
        m_texture = app->getModuleResources()->createTexture(*m_textureAsset, true);
}
