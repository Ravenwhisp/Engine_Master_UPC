#include "Globals.h"
#include "ExitApplication.h"
#include "GameObject.h"
#include "UIButton.h"
#include "Application.h"

ExitApplication::ExitApplication(UID id, GameObject* gameObject)
    : Component(id, ComponentType::EXIT_APPLICATION, gameObject)
{
}

ExitApplication::~ExitApplication()
{
    if (m_uiButton)
    {
        m_uiButton->onClick.Remove(m_onClickHandle);
    }
}

std::unique_ptr<Component> ExitApplication::clone(GameObject* newOwner) const
{
    std::unique_ptr<ExitApplication> newComponent = std::make_unique<ExitApplication>(GenerateUID(), newOwner);

    newComponent->m_uiButton = m_uiButton;

    if (newComponent->m_uiButton)
    {
        newComponent->m_onClickHandle = newComponent->m_uiButton->onClick.AddRaw(newComponent.get(), &ExitApplication::onExitApplication);
    }

    return newComponent;
}

bool ExitApplication::init()
{
    return true;
}

void ExitApplication::drawUi()
{
    ImGui::Separator();

    ImGui::Button("UI Button reference");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
        {
            Component* data = *static_cast<Component**>(payload->Data);
            m_uiButton = static_cast<UIButton*>(data);

            if (m_uiButton)
            {
                m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ExitApplication::onExitApplication);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("UI Button: %s", m_uiButton ? "Assigned" : "None (drag a UI Button here)");
}

void ExitApplication::onExitApplication()
{
    app->exitApplication();
}

rapidjson::Value ExitApplication::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::EXIT_APPLICATION), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    if (m_uiButton)
    {
        componentInfo.AddMember("UIButtonUID", m_uiButton->getID(), domTree.GetAllocator());
    }

    return componentInfo;
}

//This is wrong, but need the fixReferences script to make it good.
bool ExitApplication::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("UIButtonUID"))
    {
        m_uiButton = static_cast<UIButton*>(m_owner->GetComponent(ComponentType::UIBUTTON));
        if (m_uiButton)
        {
            m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ExitApplication::onExitApplication);
        }
    }

    return true;
}