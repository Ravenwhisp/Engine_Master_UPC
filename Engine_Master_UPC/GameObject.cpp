#include "Globals.h"
#include "GameObject.h"

GameObject::GameObject(short newUuid) : m_uuid(newUuid), m_name("New GameObject")
{
	m_transform = new Transform();
	m_transform->setParent(this);
}

GameObject::~GameObject()
{
    for (Component* component : m_components)
    {
        delete component;
    }
    m_components.clear();

    delete m_transform;
}

bool GameObject::AddComponent(Component* newComponent)
{
    if (!newComponent)
        return false;

    if (newComponent->getType() == ComponentType::TRANSFORM)
        return false;

    m_components.push_back(newComponent);
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

void GameObject::drawUI()
{
    ImGui::Checkbox("##Active", &m_active);
    ImGui::SameLine();

    char buffer[256];
    std::strncpy(buffer, m_name.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 80);
    if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
    {
        m_name = buffer;
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Tag: %s", TagToString(m_tag));
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
    ImGui::Text("Layer: %s", LayerToString(m_layer));

    m_transform->drawUi();

    for (Component* component : m_components)
    {
        component->drawUi();
    }
}
