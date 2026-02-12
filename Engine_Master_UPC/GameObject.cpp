#include "Globals.h"
#include "GameObject.h"

#include "BasicModel.h"
#include "Light.h"

GameObject::GameObject(int newUuid) : m_uuid(newUuid), m_name("New GameObject")
{
	m_transform = new Transform(rand(), this);

    //Testing duck
	BasicModel* currModel = new BasicModel(rand(), this);
    m_components.push_back(currModel);
	currModel->init();
    //////////////
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

bool GameObject::AddComponent(ComponentType componentType)
{
    if (componentType == ComponentType::COUNT)
        return false;

    if (componentType == ComponentType::TRANSFORM)

    switch (componentType)
    {
    case ComponentType::LIGHT:
        m_components.push_back(new Light(rand(), this));
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
    for (GameObject* child : m_transform->getAllChildren())
    {
        if (child->GetActive())
        {
        child->update();
        }
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
    }
    for (GameObject* child : m_transform->getAllChildren())
    {
        child->cleanUp();
    }
	return true;
}
#pragma endregion
