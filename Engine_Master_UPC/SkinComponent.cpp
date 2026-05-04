#include "Globals.h"
#include "SkinComponent.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "GameObject.h"
#include "Transform.h"
#include "SkinAsset.h"

#include <imgui.h>


namespace
{
    GameObject* FindHierarchyRoot(GameObject* go)
    {
        if (!go)
            return nullptr;

        GameObject* current = go;

        while (current)
        {
            Transform* transform = current->GetTransform();
            if (!transform)
                break;

            Transform* parentTransform = transform->getRoot();
            if (!parentTransform)
                break;

            current = parentTransform->getOwner();
        }

        return current;
    }

    GameObject* FindByNameRecursive(GameObject* go, const std::string& name)
    {
        if (!go)
            return nullptr;

        if (go->GetName() == name)
            return go;

        Transform* transform = go->GetTransform();
        if (!transform)
            return nullptr;

        for (GameObject* child : transform->getAllChildren())
        {
            if (GameObject* found = FindByNameRecursive(child, name))
                return found;
        }

        return nullptr;
    }
}

SkinComponent::SkinComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::SKIN, owner)
{
}

std::unique_ptr<Component> SkinComponent::clone(GameObject* newOwner) const
{
    auto cloned = std::make_unique<SkinComponent>(m_uuid, newOwner);

    cloned->m_skinAsset = m_skinAsset;

    cloned->m_skin.reset();
    cloned->m_jointTransforms.clear();
    cloned->m_skinBindingsResolved = false;

    cloned->setActive(isActive());
    return cloned;
}

bool SkinComponent::init()
{
    return true;
}

void SkinComponent::lateUpdate()
{
    if (m_skinAsset == INVALID_ASSET_ID)
        return;

    if (!ensureSkinLoaded())
        return;

    if (!m_skinBindingsResolved)
    {
        resolveSkinBindings();
    }
}

bool SkinComponent::cleanUp()
{
    invalidateSkinningRuntime();
    return true;
}

void SkinComponent::setSkinReference(const MD5Hash& skinUID)
{
    if (m_skinAsset == skinUID)
        return;

    m_skinAsset = skinUID;
    invalidateSkinningRuntime();
}

void SkinComponent::drawUi()
{
    ImGui::Text("Skin Asset: %s", m_skinAsset != INVALID_ASSET_ID ? m_skinAsset.c_str() : "None");
    ImGui::Text("Skin Loaded: %s", m_skin ? "Yes" : "No");
    ImGui::Text("Resolved Joints: %d", static_cast<int>(m_jointTransforms.size()));
    ImGui::Text("Skin Bindings Resolved: %s", m_skinBindingsResolved ? "Yes" : "No");
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

    invalidateSkinningRuntime();

    return true;

}

bool SkinComponent::ensureSkinLoaded()
{
    if (m_skinAsset == INVALID_ASSET_ID)
        return false;

    if (m_skin)
        return true;

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    std::shared_ptr<SkinAsset> skinAsset = moduleAssets->load<SkinAsset>(m_skinAsset);
    if (!skinAsset)
    {
        DEBUG_WARN("[SkinComponent] Could not load SkinAsset '%s'.", m_skinAsset.c_str());
        return false;
    }

    m_skin = skinAsset;
    m_skinBindingsResolved = false;

    return true;
}

bool SkinComponent::resolveSkinBindings()
{
    if (!m_owner || !m_skin)
        return false;

    GameObject* root = FindHierarchyRoot(m_owner);
    if (!root)
        return false;

    const auto& joints = m_skin->getJoints();

    m_jointTransforms.clear();
    m_jointTransforms.reserve(joints.size());

    for (const SkinJoint& joint : joints)
    {
        GameObject* jointGo = FindByNameRecursive(root, joint.nodeName);
        if (!jointGo || !jointGo->GetTransform())
        {
            DEBUG_WARN("[SkinComponent] Joint '%s' not found while resolving skin '%s'.",
                joint.nodeName.c_str(),
                m_skin->getName().c_str());

            m_jointTransforms.clear();
            m_skinBindingsResolved = false;
            return false;
        }

        m_jointTransforms.push_back(jointGo->GetTransform());
    }

    m_skinBindingsResolved = true;
    return true;
}

void SkinComponent::invalidateSkinningRuntime()
{
    m_skin.reset();
    m_jointTransforms.clear();
    m_skinBindingsResolved = false;
}