#include "Globals.h"
#include "GameObject.h"

#include "ComponentFactory.h"
#include "SceneSnapshot.h"
#include "PrefabManager.h"
#include "ModuleEditor.h"
#include "PrefabEditSession.h"
#include "ComponentType.h"

//Should not be here
#include "ModuleScene.h"
#include "Application.h"
#include "CameraComponent.h"

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

std::unique_ptr<GameObject> GameObject::clone(SceneSnapshot& snapshot) const
{
    std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(m_uuid);

	//snapshot.GameObjectMap[this] = newGameObject.get(); for now, not necessary

    newGameObject->SetName(GetName());
    newGameObject->SetActive(GetActive());
    newGameObject->SetStatic(GetStatic());
    newGameObject->SetLayer(GetLayer());
    newGameObject->SetTag(GetTag());

    //std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(*this);

    // Hay que eliminar el transform que se crea por defecto y luego clonar el transform original, para mantener la misma jerarqu�a
    newGameObject->RemoveComponent(newGameObject->GetComponent(ComponentType::TRANSFORM));

    for (const std::unique_ptr<Component>& component : m_components)
    {
        std::unique_ptr<Component> clonedComponent = component->clone(newGameObject.get());

        if (clonedComponent)
        {
			snapshot.componentMap[component->getID()] = clonedComponent.get();
            if (clonedComponent->getType() == ComponentType::TRANSFORM)
            {
                newGameObject->m_transform = static_cast<Transform*>(clonedComponent.get());
            }
            newGameObject->AddClonedComponent(std::move(clonedComponent));
        }
        else
        {
            DEBUG_WARN("[Clone] Component '%s' (type=%d, uid=%llu) returned nullptr in clone(). It will NOT exist in the cloned scene.",
                ComponentTypeToString(component->getType()),
                (int)component->getType(),
                (unsigned long long)component->getType(),
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

    GameObject* target = this;
    while (target && !PrefabManager::isPrefabInstance(target))
    {
        Transform* parentTransform = target->GetTransform()->getRoot();
        target = parentTransform ? parentTransform->getOwner() : nullptr;
    }
    if (target)
    {
        PrefabManager::markComponentAdded(target, static_cast<int>(componentType));
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
    return rawPtr;
}

bool GameObject::AddClonedComponent(std::unique_ptr<Component> component)
{
    m_components.push_back(std::move(component));
    return true;
}

bool GameObject::RemoveComponent(Component* componentToRemove)
{
    auto it = std::find_if(
        m_components.begin(),
        m_components.end(),
        [componentToRemove](const std::unique_ptr<Component>& ptr) { return ptr.get() == componentToRemove; }
    );

    if (it != m_components.end())
    {
        ComponentType removedType = (*it)->getType();
        (*it)->cleanUp();
        m_components.erase(it);
        GameObject* target = this;
        while (target && !PrefabManager::isPrefabInstance(target))
        {
            Transform* parentTransform = target->GetTransform()->getRoot();
            target = parentTransform ? parentTransform->getOwner() : nullptr;
        }
        if (target) 
        {
            PrefabManager::markComponentRemoved(target, static_cast<int>(removedType));
        }
  
        return true;
    }
    return false;
}

std::vector<Component*> GameObject::GetAllComponents() const
{
    std::vector<Component*> result = std::vector<Component*>();
    for (const std::unique_ptr<Component>& component : m_components)
    {
        result.push_back(component.get());
    }
    return result;
}

Component* GameObject::GetComponent(ComponentType type) const
{
    std::vector<Component*> result = std::vector<Component*>();
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

    for (GameObject* child : m_transform->getAllChildren())
    {
        if (child && child->GetActive())
            child->update();
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

void GameObject::drawUI()
{
#pragma region

    ImGui::Text("GameObject UUID: %llu", (unsigned long long)m_uuid);
    ImGui::Separator();

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
        ImGui::Checkbox("##Active", &m_active);

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
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Tag");
        ImGui::TableSetColumnIndex(1);
        DrawEnumCombo("##Tag", m_tag, static_cast<int>(Tag::COUNT), TagToString);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Layer");
        ImGui::TableSetColumnIndex(1);
        DrawEnumCombo("##Layer", m_layer, static_cast<int>(Layer::COUNT), LayerToString);

        ImGui::EndTable();
    }

    ImGui::Separator();

#pragma endregion

#pragma region Components
    ImGui::Text("Components");
    ImGui::Separator();

    for (size_t i = 0; i < m_components.size(); ++i)
    {
        const std::unique_ptr<Component>& component = m_components[i];
        ImGui::PushID(component->getID());

        std::string header = std::string(ComponentTypeToString(component->getType())) + " | UUID: " + std::to_string(component->getID());

        /*if (component->getType() == ComponentType::CAMERA)
        {
            CameraComponent* cameraComponent = static_cast<CameraComponent*>(component.get());
            if (app->getModuleScene()->getDefaultCamera() == cameraComponent)
            {
                header += " (Default)";
            }
        }*/

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

        bool isOpen = ImGui::TreeNodeEx("##component", flags, "%s", header.c_str());

        if (i != 0)
        {
            ImGui::SameLine(ImGui::GetContentRegionMax().x - 25);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("COMPONENT", &component, sizeof(Component));
            ImGui::Text("Component UID %s");
            ImGui::EndDragDropSource();
        }

        bool enabled = component->isActive();
        if (component->getType() != ComponentType::TRANSFORM && ImGui::Checkbox("##Active", &enabled))
        {
            component->setActive(enabled);
        }

        if (isOpen)
        {
            ImGui::Separator();

            PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
            const bool inPrefabMode = session && session->m_active && session->m_rootObject;

            const ImGuiID activeIdBefore = ImGui::GetActiveID();
            component->drawUi();
            const ImGuiID activeIdAfter = ImGui::GetActiveID();

            if (inPrefabMode && activeIdAfter != 0)
            {
                const int componentType = static_cast<int>(component->getType()); 
                GameObject* targetForOverride = app->getModuleEditor()->getSelectedGameObject();
                if (targetForOverride)
                {
                    PrefabManager::markPropertyOverride(
                        targetForOverride, componentType, "properties");
                }
            }

            ImGui::Separator();

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

void GameObject::onTransformChange()
{
    for (const auto& component : m_components)
    {
        component->onTransformChange();
    }
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

    const PrefabInstanceData* instanceData = PrefabManager::getInstanceData(this);
    if (instanceData && !instanceData->m_prefabName.empty())
    {
        rapidjson::Value prefabLink(rapidjson::kObjectType);
        rapidjson::Value prefabName(instanceData->m_prefabName.c_str(), domTree.GetAllocator());
        prefabLink.AddMember("PrefabName", prefabName, domTree.GetAllocator());
        prefabLink.AddMember("PrefabUID", instanceData->m_prefabUID, domTree.GetAllocator());
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
        }
    }

    return true;
}

#pragma endregion