#include "Globals.h"
#include "UIButton.h"

#include <imgui.h>

#include "UIImage.h"
#include "SceneReferenceResolver.h"

UIButton::UIButton(UID id, GameObject* owner)
    : Component(id, ComponentType::UIBUTTON, owner)
{
}

std::unique_ptr<Component> UIButton::clone(GameObject* newOwner) const
{
    std::unique_ptr<UIButton> clonedButton = std::make_unique<UIButton>(m_uuid, newOwner);

    clonedButton->setActive(this->isActive());
    clonedButton->m_targetGraphic = this->m_targetGraphic;
    clonedButton->m_targetGraphicUid = this->m_targetGraphicUid;
    // Note: onClick listeners are not cloned, as they are typically set up in code after instantiation

	return clonedButton;
}


void UIButton::onPointerUp(PointerEventData& /*data*/)
{
    m_isPressed = false;
}


void UIButton::onPointerClick(PointerEventData& /*data*/)
{
    press();
}


void UIButton::press()
{
    if (!isActive()) return;
    onClick.Broadcast();
    DEBUG_LOG("UIButton pressed");
}


void UIButton::drawUi()
{
    ImGui::Text("UIButton");
    ImGui::Separator();

    ImGui::Button("TargetGraphic reference");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
        {
            Component* data = *static_cast<Component**>(payload->Data);
            UIImage* image = static_cast<UIImage*>(data);
            if (image) 
            {
                setTargetGraphic(image);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Target graphic slot
    ImGui::Text("Target Graphic: %s", m_targetGraphic ? "Assigned" : "None (drag a UIImage GO here)");

    // onClick listener count (read-only info)
    ImGui::Text("onClick listeners: %zu", onClick.GetSize());
}

rapidjson::Value UIButton::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::UIBUTTON), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("TargetGraphicUID", (uint64_t)m_targetGraphicUid, domTree.GetAllocator());

    return componentInfo;
}

bool UIButton::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TargetGraphicUID"))
    {
        m_targetGraphicUid = (UID)componentInfo["TargetGraphicUID"].GetUint64();
    }

    m_targetGraphic = nullptr;
    return true;
}

void UIButton::fixReferences(const SceneReferenceResolver& resolver)
{
    if (m_targetGraphicUid == 0)
    {
        m_targetGraphic = nullptr;
        return;
    }

    m_targetGraphic = static_cast<UIImage*>(resolver.getClonedComponent(m_targetGraphicUid));
}
