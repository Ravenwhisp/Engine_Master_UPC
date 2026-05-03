#include "Globals.h"
#include "SkinComponent.h"

#include <imgui.h>

SkinComponent::SkinComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::SKIN, owner)
{
}

std::unique_ptr<Component> SkinComponent::clone(GameObject* newOwner) const
{
    auto cloned = std::make_unique<SkinComponent>(m_uuid, newOwner);
    cloned->setActive(isActive());
    return cloned;
}

bool SkinComponent::init()
{
    return true;
}

void SkinComponent::lateUpdate()
{
}

bool SkinComponent::cleanUp()
{
    return true;
}

void SkinComponent::setSkinReference(const MD5Hash& skinUID)
{
    if (m_skinAsset == skinUID)
        return;

    m_skinAsset = skinUID;
}

void SkinComponent::drawUi()
{
    ImGui::Text("Skin Asset: %s", m_skinAsset != INVALID_ASSET_ID ? m_skinAsset.c_str() : "None");
    ImGui::TextDisabled("SkinComponent runtime data will be added in the next commits.");
}

rapidjson::Value SkinComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", static_cast<int>(getType()), domTree.GetAllocator());
    componentInfo.AddMember("Active", isActive(), domTree.GetAllocator());
    componentInfo.AddMember("SkinAssetId", rapidjson::Value(m_skinAsset.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    return componentInfo;
}

bool SkinComponent::deserializeJSON(const rapidjson::Value& componentValue)
{
    if (componentValue.HasMember("Active") && componentValue["Active"].IsBool())
    {
        setActive(componentValue["Active"].GetBool());
    }

    if (componentValue.HasMember("SkinAssetId") && componentValue["SkinAssetId"].IsString())
    {
        m_skinAsset = componentValue["SkinAssetId"].GetString();
    }
    else
    {
        m_skinAsset = INVALID_ASSET_ID;
    }

    return true;

}