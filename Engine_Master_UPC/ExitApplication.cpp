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
    std::unique_ptr<ExitApplication> newComponent = std::make_unique<ExitApplication>(m_uuid, newOwner);

    newComponent->setActive(this->isActive());
    newComponent->m_uiButton = m_uiButton;
    newComponent->m_uiButtonUid = m_uiButtonUid;

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
            UIButton* button = static_cast<UIButton*>(data);

            if (button)
            {
                if (m_uiButton)
                {
                    m_uiButton->onClick.Remove(m_onClickHandle);
                }

                m_uiButton = button;
                m_uiButtonUid = m_uiButton->getID();
                m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ExitApplication::onExitApplication);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("UI Button: %s", m_uiButton ? "Assigned" : "None (drag a UI Button here)");
}

void ExitApplication::onExitApplication()
{
    app->requestApplicationExit();
}

rapidjson::Value ExitApplication::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::EXIT_APPLICATION), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("UIButtonUID", (uint64_t)m_uiButtonUid, domTree.GetAllocator());

    return componentInfo;
}

bool ExitApplication::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("UIButtonUID"))
    {
        m_uiButtonUid = (UID)componentInfo["UIButtonUID"].GetUint64();
    }

    m_uiButton = nullptr;

    return true;
}

void ExitApplication::fixReferences(const std::unordered_map<UID, Component*>& referenceMap)
{
    if (m_uiButton)
    {
        m_uiButton->onClick.Remove(m_onClickHandle);
        m_uiButton = nullptr;
    }

    if (m_uiButtonUid == 0)
    {
        return;
    }

    auto it = referenceMap.find(m_uiButtonUid);
    if (it != referenceMap.end())
    {
        m_uiButton = static_cast<UIButton*>(it->second);
        m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ExitApplication::onExitApplication);
    }
}