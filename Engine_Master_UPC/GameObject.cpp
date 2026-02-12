#include "Globals.h"
#include "GameObject.h"

#include "BasicModel.h"
#include "Light.h"

GameObject::GameObject(int newUuid) : m_uuid(newUuid), m_name("New GameObject")
{
    m_components.push_back(m_transform = new Transform(rand(), this));

    //Testing duck
	BasicModel* currModel = new BasicModel(rand(), this);
    m_components.push_back(currModel);
	currModel->init();
    //////////////
}

GameObject::~GameObject()
{

}

bool GameObject::AddComponent(ComponentType componentType)
{
    switch (componentType)
    {
        case ComponentType::LIGHT:
            m_components.push_back(new Light(rand(), this));
            break;
        case ComponentType::MODEL:
            m_components.push_back(new BasicModel(rand(), this));
            break;

        case ComponentType::TRANSFORM:
        case ComponentType::COUNT:
            return false;
            break;

        default:
            return false;
            break;
    }

    return true;
}

bool GameObject::RemoveComponent(Component* componentToRemove)
{
    auto it = std::find(m_components.begin(), m_components.end(), componentToRemove);
    if (it != m_components.end())
    {
        delete* it;
        m_components.erase(it);
        return true;
    }
    return false;
}

#pragma region GameLoop
bool GameObject::init()
{
    for (Component* component : m_components)
    {
        component->init();
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        child->init();
    }
    return true;
}

void GameObject::update() {
    for (Component* component : m_components)
    {
        component->update();
	}
}

void GameObject::preRender()
{
    for (Component* component : m_components)
    {
        component->preRender();
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        if (child->GetActive())
        {
            child->preRender();
        }
    }
}

void GameObject::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
    for (Component* component : m_components)
    {
        component->render(commandList, viewMatrix, projectionMatrix);
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        if (child->GetActive())
        {
            child->render(commandList, viewMatrix, projectionMatrix);
        }
    }
}

void GameObject::postRender()
{
    for (Component* component : m_components)
    {
        component->postRender();
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        if (child->GetActive())
        {
            child->postRender();
        }
    }
}

bool GameObject::cleanUp()
{
    for (Component* component : m_components)
    {
        component->cleanUp();
        delete component;
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
    ImGui::Text("GameObject UUID: %d", m_uuid);
    ImGui::Separator();

    ImGui::Checkbox("Active", &m_active);

    char buffer[256];
    std::strncpy(buffer, m_name.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText("Name", buffer, sizeof(buffer)))
        m_name = buffer;

    float totalWidth = ImGui::GetContentRegionAvail().x;
    float comboWidth = totalWidth * 0.40f;

    ImGui::PushItemWidth(comboWidth);
    DrawEnumCombo("Tag", m_tag, static_cast<int>(Tag::COUNT), TagToString);
    ImGui::PopItemWidth();

    ImGui::SameLine();

    ImGui::PushItemWidth(comboWidth);
    DrawEnumCombo("Layer", m_layer, static_cast<int>(Layer::COUNT), LayerToString);
    ImGui::PopItemWidth();

    ImGui::Separator();
#pragma endregion
    
#pragma region Components
    ImGui::Text("Components");
    ImGui::Separator();

    for (size_t i = 0; i < m_components.size(); ++i)
    {
        Component* component = m_components[i];

        ImGui::PushID(component->getID());

        std::string header = std::string(ComponentTypeToString(component->getType())) + " | UUID: " + std::to_string(component->getID());

        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            component->drawUi();

            ImGui::Separator();

            if (component->getType() != ComponentType::TRANSFORM)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                if (ImGui::Button("Remove Component"))
                {
                    RemoveComponent(component);
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                    break;
                }

                ImGui::PopStyleColor();
            }
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

#pragma endregion
