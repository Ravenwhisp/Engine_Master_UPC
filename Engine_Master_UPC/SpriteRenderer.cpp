#include "Globals.h"
#include "SpriteRenderer.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleAssets.h"
#include "Texture.h"

SpriteRenderer::SpriteRenderer(UID id, GameObject* owner) : Component(id, ComponentType::SPRITE_RENDERER, owner) { }

std::unique_ptr<Component> SpriteRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<SpriteRenderer> cloned = std::make_unique<SpriteRenderer>(m_uuid, newOwner);

    cloned->m_textureAssetId = m_textureAssetId;
    cloned->m_gpuTexture = m_gpuTexture;
    cloned->m_texture = cloned->m_gpuTexture.get();
    cloned->m_textureAsset = m_textureAsset;
    cloned->m_loadRequested = false;
    cloned->m_lookAtCamera = m_lookAtCamera;
    cloned->setActive(this->isActive());

    return cloned;
}

bool SpriteRenderer::consumeLoadRequest()
{
    const bool wasRequested = m_loadRequested;
    m_loadRequested = false;
    return wasRequested;
}

void SpriteRenderer::setGpuTexture(std::shared_ptr<Texture> texture)
{
    m_gpuTexture = texture;
    m_texture = m_gpuTexture.get();
}

void SpriteRenderer::drawUi()
{
    ImGui::Text("SpriteRenderer");

    ImGui::Button("Drop Here the Texture");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
        {
            const MD5Hash* data = static_cast<const MD5Hash*>(payload->Data);
            m_textureAssetId = *data;
            m_texture = nullptr;
            m_gpuTexture = nullptr;
            m_textureAsset = app->getModuleAssets()->load<TextureAsset>(*data);
            DEBUG_LOG("SpriteRenderer drop: assetId=%s", data->c_str());
            DEBUG_LOG("SpriteRenderer load result: %s", m_textureAsset ? "OK" : "NULL");
            if (m_textureAsset)
            {
                m_loadRequested = true;
                DEBUG_LOG("SpriteRenderer load requested: %s", m_loadRequested ? "YES" : "NO");
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::Text("Loaded: %s", (m_texture != nullptr) ? "YES" : "NO");

    ImGui::Checkbox("Look At Camera", &m_lookAtCamera);
}

rapidjson::Value SpriteRenderer::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::SPRITE_RENDERER), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("TextureAssetId", rapidjson::Value(m_textureAssetId.c_str(), domTree.GetAllocator()), domTree.GetAllocator());
    componentInfo.AddMember("LookAtCamera", bool(this->m_lookAtCamera), domTree.GetAllocator());

    return componentInfo;
}

bool SpriteRenderer::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TextureAssetId"))
    {
        m_textureAssetId = componentInfo["TextureAssetId"].GetString();

        m_texture = nullptr;
        m_gpuTexture = nullptr;
        m_textureAsset = app->getModuleAssets()->load<TextureAsset>(m_textureAssetId);

        if (m_textureAsset)
        {
            m_loadRequested = true;
        }
    }

    if (componentInfo.HasMember("LookAtCamera"))
    {
        m_lookAtCamera = componentInfo["LookAtCamera"].GetBool();
    }

    return true;
}