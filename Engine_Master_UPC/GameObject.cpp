#include "Globals.h"
#include "GameObject.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "ComponentType.h"
#include "Component.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "SkinComponent.h"
#include "CameraComponent.h"
#include "ScriptComponent.h"
#include "ComponentFactory.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <imgui.h>

#include "Quadtree.h"


GameObject::GameObject(UID newUuid) : m_uuid(newUuid), m_name("New GameObject")
{
    auto transform = std::make_unique<Transform>(GenerateUID(), this);
    m_transform = transform.get();
    m_components.push_back(std::move(transform));
}

GameObject::GameObject(UID newUuid, UID transformUid) : m_uuid(newUuid), m_name("New GameObject")
{
    auto transform = std::make_unique<Transform>(transformUid, this);
    m_transform = transform.get();
    m_components.push_back(std::move(transform));
}

GameObject::~GameObject()
{

}

std::unique_ptr<GameObject> GameObject::clone() const
{
    auto newGameObject = std::make_unique<GameObject>(m_uuid);

    newGameObject->SetName(GetName());
    newGameObject->SetActive(GetActive());
    newGameObject->SetStatic(GetStatic());
    newGameObject->SetLayer(GetLayer());
    newGameObject->SetTag(GetTag());

    newGameObject->GetTransform()->setRoot(nullptr);
    newGameObject->cleanUp();

    for (const std::unique_ptr<Component>& component : m_components)
    {
        auto clonedComponent = component->clone(newGameObject.get());

        if (clonedComponent)
        {
            if (clonedComponent->getType() == ComponentType::TRANSFORM)
            {
                newGameObject->m_transform = static_cast<Transform*>(clonedComponent.get());
            }
            newGameObject->AddClonedComponent(std::move(clonedComponent));
        }
        else
        {
            DEBUG_WARN("[Clone] Component '%s' failed to clone (uid=%llu)",
                ComponentTypeToString(component->getType()),
                (unsigned long long)component->getID());
        }
    }

    return newGameObject;
}

bool GameObject::AddComponent(ComponentType componentType)
{
    if (componentType == ComponentType::TRANSFORM || componentType == ComponentType::COUNT)
    {
        return false;
    }

    std::unique_ptr<Component> newComponent = ComponentFactory::create(componentType, this);
    if (!newComponent)
    {
        return false;
    }
    newComponent->init();
    m_components.push_back(std::move(newComponent));
    app->getModuleScene()->getScene()->markDirty();

    GameObject* target = this;
    while (target && !target->IsPrefabInstance())
    {
        Transform* parentTransform = target->GetTransform()->getRoot();
        target = parentTransform ? parentTransform->getOwner() : nullptr;
    }
    if (target)
    {
        auto& added = target->GetPrefabInfo().m_overrides.m_addedComponentTypes;
        const int ct = static_cast<int>(componentType);
        if (std::find(added.begin(), added.end(), ct) == added.end())
        {
            added.push_back(ct);
        }
        auto& removed = target->GetPrefabInfo().m_overrides.m_removedComponentTypes;
        removed.erase(std::remove(removed.begin(), removed.end(), ct), removed.end());
    }

    return true;
}

Component* GameObject::AddComponentWithUID(const ComponentType componentType, UID id)
{
    if (componentType == ComponentType::TRANSFORM || componentType == ComponentType::COUNT)
    {
        return nullptr;
    }

    std::unique_ptr<Component> newComponent = ComponentFactory::createWithUID(componentType, id, this);
    if (!newComponent)
    {
        return nullptr;
    }

    Component* rawPtr = newComponent.get();

    m_components.push_back(std::move(newComponent));
    app->getModuleScene()->getScene()->markDirty();
    return rawPtr;
}

bool GameObject::AddClonedComponent(std::unique_ptr<Component> component)
{
    m_components.push_back(std::move(component));
    return true;
}

bool GameObject::RemoveComponent(Component* componentToRemove)
{
    auto it = std::find_if(m_components.begin(), m_components.end(), [componentToRemove](const std::unique_ptr<Component>& ptr) { return ptr.get() == componentToRemove; });

    if (it == m_components.end())
    {
        return false;
    }

    ComponentType removedType = (*it)->getType();

    // Resolve the prefab root BEFORE cleanup — cleanUp() may invalidate
    // parent pointers if it releases references or triggers notifications.
    GameObject* target = this;
    while (target && !target->IsPrefabInstance())
    {
        Transform* parentTransform = target->GetTransform()->getRoot();
        target = parentTransform ? parentTransform->getOwner() : nullptr;
    }

    (*it)->cleanUp();
    m_components.erase(it);
    app->getModuleScene()->getScene()->markDirty();

    if (target)
    {
        const int ct = static_cast<int>(removedType);
        auto& removed = target->GetPrefabInfo().m_overrides.m_removedComponentTypes;
        if (std::find(removed.begin(), removed.end(), ct) == removed.end())
        {
            removed.push_back(ct);
        }
        auto& added = target->GetPrefabInfo().m_overrides.m_addedComponentTypes;
        added.erase(std::remove(added.begin(), added.end(), ct), added.end());
        target->GetPrefabInfo().m_overrides.m_modifiedProperties.erase(ct);
    }

    return true;
}

std::vector<Component*> GameObject::GetAllComponents() const
{
    std::vector<Component*> result = std::vector<Component*>();;
    for (const std::unique_ptr<Component>& component : m_components)
    {
        result.push_back(component.get());
    }
    return result;
}

Component* GameObject::GetComponent(ComponentType type) const
{
    for (const std::unique_ptr<Component>& component : m_components)
    {
        if (component && component->getType() == type)
        {
            return component.get();
        }
    }
    return nullptr;
}

#pragma region Properties

bool GameObject::IsActiveInWindowHierarchy() const
{
    if (!m_active)
    {
        return false;
    }

    Transform* parentTransform = m_transform->getRoot();
    if (parentTransform == nullptr)
    {
        return true;
    }

    GameObject* parent = parentTransform->getOwner();
    if (parent != nullptr)
    {
        return parent->IsActiveInWindowHierarchy();
    }

    return true;
}

void GameObject::SetActive(bool newActive)
{
    m_active = newActive;
    app->getModuleScene()->getScene()->markDirty();
}

#pragma endregion

#pragma region GameLoop
bool GameObject::init()
{
    for (const std::unique_ptr<Component>& component : m_components)
    {
        component->init();
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        child->init();
    }
    return true;
}

void GameObject::update()
{
    if (!IsActiveInWindowHierarchy())
    {
        return;
    }

    for (const std::unique_ptr<Component>& component : m_components)
    {
        if (component && component->isActive())
        {
            component->update();
        }
    }
}

void GameObject::lateUpdate()
{
    if (!IsActiveInWindowHierarchy())
    {
        return;
    }

    for (const std::unique_ptr<Component>& component : m_components)
    {
        if (component && component->isActive())
        {
            component->lateUpdate();
        }
    }
}

bool GameObject::cleanUp()
{
    for (const std::unique_ptr<Component>& component : m_components)
    {
        component->cleanUp();
    }
    m_components.clear();

    return true;
}
#pragma endregion

#pragma region Editor
template<typename EnumType>
bool DrawEnumCombo(const char* label, EnumType& currentValue, int count, const char* (*toStringFunc)(EnumType))
{
    int currentIndex = static_cast<int>(currentValue);

    if (ImGui::BeginCombo(label, toStringFunc(currentValue)))
    {
        for (int i = 0; i < count; ++i)
        {
            EnumType value = static_cast<EnumType>(i);
            bool isSelected = (i == currentIndex);

            if (ImGui::Selectable(toStringFunc(value), isSelected))
            {
                currentValue = value;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
        return true;
    }

    return false;
}

void GameObject::markGameObjectPropertyOverride(const char* propertyName)
{
    // Walk up to the nearest prefab instance root (same pattern used elsewhere)
    GameObject* target = this;
    while (target && !target->IsPrefabInstance())
    {
        Transform* parentTransform = target->GetTransform()->getRoot();
        target = parentTransform ? parentTransform->getOwner() : nullptr;
    }
    if (target)
    {
        // -1 is a safe sentinel key for GameObject-level (non-component) properties
        target->GetPrefabInfo().m_overrides.m_modifiedProperties[-1].insert(propertyName);
    }
}

void GameObject::drawUI()
{
#pragma region

    ImGui::Text("GameObject UUID: %llu", (unsigned long long)m_uuid);
    ImGui::Separator();
    const bool inPrefabMode = app->getModuleEditor()->isInPrefabEditMode();

    if (ImGui::BeginTable("GameObjectWindowInspector", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("UUID");

        ImGui::TableSetColumnIndex(1);

        std::string uuidStr = std::to_string(m_uuid);

        ImGui::Text("%s", uuidStr.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImGui::SetClipboardText(uuidStr.c_str());
            DEBUG_LOG("UUID %s copied.", uuidStr.c_str());
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(Double-click to copy)");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Active");
        ImGui::TableSetColumnIndex(1);
        bool prevActive = m_active;
        bool active = m_active;

        if (ImGui::Checkbox("##Active", &active))
        {
            SetActive(active);
            
            if (inPrefabMode && active != prevActive)
    		{
        		markGameObjectPropertyOverride("active");
    		}
        }

        char buffer[256];
        std::strncpy(buffer, m_name.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Name");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
        {
            m_name = buffer;
            if (inPrefabMode)
                markGameObjectPropertyOverride("name");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Tag");
        ImGui::TableSetColumnIndex(1);
        {
            Tag prevTag = m_tag;
            DrawEnumCombo("##Tag", m_tag, static_cast<int>(Tag::COUNT), TagToString);
            if (inPrefabMode && m_tag != prevTag)
                markGameObjectPropertyOverride("tag");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Layer");
        ImGui::TableSetColumnIndex(1);
        {
            static Layer s_pendingLayer = Layer::DEFAULT;
            static GameObject* s_pendingLayerTarget = nullptr;

            Layer previousLayer = m_layer;
            if (DrawEnumCombo("##Layer", m_layer, static_cast<int>(Layer::COUNT), LayerToString))
            {
                if (m_layer != previousLayer && !m_transform->getAllChildren().empty())
                {
                    s_pendingLayer = m_layer;
                    s_pendingLayerTarget = this;
                    m_layer = previousLayer; // revert until user decides
                    ImGui::OpenPopup("##LayerChildrenPopup");
                }
                else if (inPrefabMode && m_layer != previousLayer)
                {
                    markGameObjectPropertyOverride("layer");
                }
            }

            if (ImGui::BeginPopupModal("##LayerChildrenPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::Text("Change layer to '%s'.", LayerToString(s_pendingLayer));
                ImGui::Text("Do you want to apply it to all children as well?");
                ImGui::Spacing();

                if (ImGui::Button("Yes, apply to children"))
                {
                    if (s_pendingLayerTarget)
                    {
                        std::function<void(GameObject*)> applyRecursive = [&](GameObject* go)
                            {
                                go->SetLayer(s_pendingLayer);
                                for (GameObject* child : go->GetTransform()->getAllChildren())
                                    applyRecursive(child);
                            };
                        applyRecursive(s_pendingLayerTarget);
                        s_pendingLayerTarget = nullptr;
                    }
                    if (inPrefabMode)
                        markGameObjectPropertyOverride("layer");
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("No, only this object"))
                {
                    if (s_pendingLayerTarget)
                    {
                        s_pendingLayerTarget->SetLayer(s_pendingLayer);
                        s_pendingLayerTarget = nullptr;
                    }
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    s_pendingLayerTarget = nullptr;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

#pragma endregion

#pragma region Components
    int pendingMoveFrom = -1;
    int pendingMoveTo = -1;

    for (size_t i = 0; i < m_components.size(); ++i)
    {
        const std::unique_ptr<Component>& component = m_components[i];
        ImGui::PushID(static_cast<int>(component->getID()));

        std::string header = std::string(ComponentTypeToString(component->getType())); // + " | UUID: " + std::to_string(component->getID());

        if (component->getType() == ComponentType::SCRIPT)
        {
            ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(component.get());

            if (scriptComponent->getScript())
            {
                const std::string& scriptName = scriptComponent->getScriptName();
                header += " (" + scriptName + ")";

            }
            else
            {
                header += " (Not instantiated)";
            }
        }

        if (component->getType() == ComponentType::CAMERA)
        {
            CameraComponent* cameraComponent = static_cast<CameraComponent*>(component.get());
            if (app->getModuleScene()->getScene()->getDefaultCamera() == cameraComponent)
            {
                header += " (Default)";
            }
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

        bool isOpen = ImGui::TreeNodeEx("##component", flags, "%s", header.c_str());

        ImVec2 headerMin = ImGui::GetItemRectMin();
        ImVec2 headerMax = ImGui::GetItemRectMax();
        ImVec2 cursorAfterHeader = ImGui::GetCursorScreenPos();

        const bool canReorder = (component->getType() != ComponentType::TRANSFORM);

        if (canReorder && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            Component* raw = component.get();
            ImGui::SetDragDropPayload("COMPONENT", &raw, sizeof(Component*));
            ImGui::Text("Move %s", header.c_str());
            ImGui::EndDragDropSource();
        }

        if (canReorder)
        {
            const float dropZoneHeight = 5.0f;
            const float fullWidth = headerMax.x - headerMin.x;

            // TOP DROP ZONE
            ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMin.y));
            ImGui::InvisibleButton("##drop_above", ImVec2(fullWidth, dropZoneHeight));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
                {
                    Component* sourceComponent = *reinterpret_cast<Component* const*>(payload->Data);
                    int sourceIndex = findComponentIndex(sourceComponent);
                    int targetIndex = static_cast<int>(i);

                    if (sourceIndex != -1)
                    {
                        pendingMoveFrom = sourceIndex;
                        pendingMoveTo = targetIndex;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // BOTTOM DROP ZONE
            ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMax.y - dropZoneHeight));
            ImGui::InvisibleButton("##drop_below", ImVec2(fullWidth, dropZoneHeight));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
                {
                    Component* sourceComponent = *reinterpret_cast<Component* const*>(payload->Data);
                    int sourceIndex = findComponentIndex(sourceComponent);
                    int targetIndex = static_cast<int>(i + 1);

                    if (sourceIndex != -1)
                    {
                        pendingMoveFrom = sourceIndex;
                        pendingMoveTo = targetIndex;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SetCursorScreenPos(cursorAfterHeader);
        }

        if (component->getType() != ComponentType::TRANSFORM)
        {
            ImGui::SetCursorScreenPos(ImVec2(headerMax.x - 24.0f, headerMin.y));

            bool enabled = component->isActive();
            if (ImGui::Checkbox("##Active", &enabled))
            {
                component->setActive(enabled);
                // Toggling a component's active state is a component-level override
                if (inPrefabMode)
                {
                    const int componentType = static_cast<int>(component->getType());
                    GameObject* targetForOverride = app->getModuleEditor()->getSelectedGameObject();
                    if (targetForOverride && targetForOverride->IsPrefabInstance())
                    {
                        targetForOverride->GetPrefabInfo()
                            .m_overrides.m_modifiedProperties[componentType].insert("active");
                    }
                }
            }

            ImGui::SetCursorScreenPos(cursorAfterHeader);
        }

        if (isOpen)
        {
            const ImGuiID activeIdBefore = ImGui::GetActiveID();
            component->drawUi();
            const ImGuiID activeIdAfter = ImGui::GetActiveID();

            // If a widget inside this component was interacted with while in prefab
            if (inPrefabMode && activeIdAfter != 0 && activeIdAfter != activeIdBefore)
            {
                const int componentType = static_cast<int>(component->getType());
                GameObject* targetForOverride = app->getModuleEditor()->getSelectedGameObject();
                if (targetForOverride && targetForOverride->IsPrefabInstance())
                {
                    targetForOverride->GetPrefabInfo()
                        .m_overrides.m_modifiedProperties[componentType].insert("properties");
                }
            }


            if (component->getType() != ComponentType::TRANSFORM)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                if (ImGui::Button("Remove Component"))
                {
                    RemoveComponent(component.get());
                    ImGui::PopStyleColor();
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }

                ImGui::PopStyleColor();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (pendingMoveFrom != -1 && pendingMoveTo != -1)
    {
        moveComponent(static_cast<size_t>(pendingMoveFrom), static_cast<size_t>(pendingMoveTo));
    }

    ImGui::Separator();

    if (ImGui::BeginCombo("Add Component", "Select"))
    {
        for (int i = 0; i < static_cast<int>(ComponentType::COUNT); ++i)
        {
            ComponentType type = static_cast<ComponentType>(i);

            if (type == ComponentType::TRANSFORM)
                continue;

            if (ImGui::Selectable(ComponentTypeToString(type)))
            {
                AddComponent(type);
            }
        }

        ImGui::EndCombo();
    }
#pragma endregion
}

void GameObject::moveComponent(size_t fromIndex, size_t toIndex)
{
    if (fromIndex >= m_components.size())
    {
        return;
    }

    if (toIndex > m_components.size())
    {
        return;
    }

    if (fromIndex == toIndex)
    {
        return;
    }

    if (fromIndex == 0 || toIndex == 0)
    {
        return;
    }

    std::unique_ptr<Component> movedComponent = std::move(m_components[fromIndex]);
    m_components.erase(m_components.begin() + static_cast<std::ptrdiff_t>(fromIndex));

    if (fromIndex < toIndex)
    {
        --toIndex;
    }

    m_components.insert(m_components.begin() + static_cast<std::ptrdiff_t>(toIndex), std::move(movedComponent));
}

int GameObject::findComponentIndex(const Component* component) const
{
    for (size_t i = 0; i < m_components.size(); ++i)
    {
        if (m_components[i].get() == component)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void GameObject::onTransformChange()
{
    for (const auto& component : m_components)
    {
        component->onTransformChange();
    }
    app->getModuleScene()->getQuadtree()->move(*this);
}

#pragma endregion

#pragma region Persistence

rapidjson::Value GameObject::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value gameObjectInfo(rapidjson::kObjectType);

    gameObjectInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    {
        Transform* parentTransform = m_transform->getRoot();
        if (parentTransform)
        {
            gameObjectInfo.AddMember("ParentUID", parentTransform->getOwner()->GetID(), domTree.GetAllocator());
        }
        else {
            gameObjectInfo.AddMember("ParentUID", 0, domTree.GetAllocator());
        }
    }


    rapidjson::Value name(m_name.c_str(), domTree.GetAllocator());
    gameObjectInfo.AddMember("Name", name, domTree.GetAllocator());

    gameObjectInfo.AddMember("Active", m_active, domTree.GetAllocator());
    gameObjectInfo.AddMember("Static", m_isStatic, domTree.GetAllocator());

    rapidjson::Value layer(LayerToString(m_layer), domTree.GetAllocator());
    gameObjectInfo.AddMember("Layer", layer, domTree.GetAllocator());

    rapidjson::Value tag(TagToString(m_tag), domTree.GetAllocator());
    gameObjectInfo.AddMember("Tag", tag, domTree.GetAllocator());

    gameObjectInfo.AddMember("Transform", m_transform->getJSON(domTree), domTree.GetAllocator());

    if (m_prefabInfo.isInstance())
    {
        rapidjson::Value prefabLink(rapidjson::kObjectType);

        rapidjson::Value sourcePath(m_prefabInfo.m_sourcePath.string().c_str(), domTree.GetAllocator());
        prefabLink.AddMember("SourcePath", sourcePath, domTree.GetAllocator());

        if (!m_prefabInfo.m_assetUID.empty())
        {
            rapidjson::Value assetUID(m_prefabInfo.m_assetUID.c_str(), domTree.GetAllocator());
            prefabLink.AddMember("AssetUID", assetUID, domTree.GetAllocator());
        }

        gameObjectInfo.AddMember("PrefabLink", prefabLink, domTree.GetAllocator());
    }

    // Components serialization //
    {
        rapidjson::Value componentsData(rapidjson::kArrayType);

        for (const std::unique_ptr<Component>& component : m_components)
        {
            if (component->getType() == ComponentType::TRANSFORM)
                continue;

            componentsData.PushBack(component->getJSON(domTree), domTree.GetAllocator());
        }

        gameObjectInfo.AddMember("Components", componentsData, domTree.GetAllocator());
    }

    return gameObjectInfo;
}

bool GameObject::deserializeJSON(const rapidjson::Value& gameObjectJson, uint64_t& parentUid)
{
    parentUid = gameObjectJson["ParentUID"].GetUint64();
    m_name = gameObjectJson["Name"].GetString();

    m_active = gameObjectJson["Active"].GetBool();
    m_isStatic = gameObjectJson["Static"].GetBool();
    m_layer = StringToLayer(gameObjectJson["Layer"].GetString());
    m_tag = StringToTag(gameObjectJson["Tag"].GetString());

    const auto& transform = gameObjectJson["Transform"];

    const auto& position = transform["Position"].GetArray();
    m_transform->setPosition(Vector3(position[0].GetFloat(), position[1].GetFloat(), position[2].GetFloat()));

    const auto& rotation = transform["Rotation"].GetArray();
    m_transform->setRotation(Quaternion(rotation[0].GetFloat(), rotation[1].GetFloat(), rotation[2].GetFloat(), rotation[3].GetFloat()));

    const auto& scale = transform["Scale"].GetArray();
    m_transform->setScale(Vector3(scale[0].GetFloat(), scale[1].GetFloat(), scale[2].GetFloat()));

    if (gameObjectJson.HasMember("PrefabLink") && gameObjectJson["PrefabLink"].IsObject())
    {
        const auto& pl = gameObjectJson["PrefabLink"];
        // Old files may have PrefabName/PrefabUID keys — we simply ignore them now.
        if (pl.HasMember("SourcePath") && pl["SourcePath"].IsString())
            m_prefabInfo.m_sourcePath = pl["SourcePath"].GetString();
        if (pl.HasMember("AssetUID") && pl["AssetUID"].IsString())
            m_prefabInfo.m_assetUID = pl["AssetUID"].GetString();
    }
    
    const auto& components = gameObjectJson["Components"].GetArray();
    for (auto& componentJson : components)
    {
        const uint64_t componentUid = componentJson["UID"].GetUint64();
        const ComponentType componentType = (ComponentType)componentJson["ComponentType"].GetInt();

        Component* newComponent = AddComponentWithUID(componentType, (UID)componentUid);
        if (newComponent)
        {
            if (componentJson.HasMember("Active") && componentJson["Active"].IsBool())
            {
                newComponent->setActive(componentJson["Active"].GetBool());
            }
            else
            {
                newComponent->setActive(true);
            }

            newComponent->deserializeJSON(componentJson);
            newComponent->init();
        }
    }

    MeshRenderer* meshRenderer = GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    SkinComponent* skinComponent = GetComponentAs<SkinComponent>(ComponentType::SKIN);

    if (meshRenderer &&
        meshRenderer->getSkinReference() != INVALID_ASSET_ID &&
        !skinComponent)
    {
        skinComponent = static_cast<SkinComponent*>(
            AddComponentWithUID(ComponentType::SKIN, GenerateUID()));

        if (skinComponent)
        {
            skinComponent->setSkinReference(meshRenderer->getSkinReference());
            skinComponent->init();
        }
    }
    else if (meshRenderer &&
        meshRenderer->getSkinReference() != INVALID_ASSET_ID &&
        skinComponent &&
        skinComponent->getSkinReference() == INVALID_ASSET_ID)
    {
        skinComponent->setSkinReference(meshRenderer->getSkinReference());
    }

    return true;
}

#pragma endregion