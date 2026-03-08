#include "Globals.h"
#include "UIButton.h"
#include "UIImage.h"
#include <imgui.h>

UIButton::UIButton(UID id, GameObject* owner)
    : Component(id, ComponentType::UIBUTTON, owner)
{
}

std::unique_ptr<Component> UIButton::clone(GameObject* newOwner) const
{
    std::unique_ptr<UIButton> clonedButton = std::make_unique<UIButton>(m_uuid, newOwner);

    clonedButton->setActive(this->isActive());
    clonedButton->setTargetGraphic(this->getTargetGraphic());
    // Note: onClick listeners are not cloned, as they are typically set up in code after instantiation

	return clonedButton;
}

void UIButton::fixReferences(const std::unordered_map<Component*, Component*>& referenceMap)
{
    if (m_targetGraphic)
    {
        auto it = referenceMap.find(m_targetGraphic);
        if (it != referenceMap.end())
        {
            m_targetGraphic = static_cast<UIImage*>(it->second);
        }
        else
        {
            m_targetGraphic = nullptr; // Clear reference if not found
        }
	}
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
            Component* data = static_cast<Component*>(payload->Data);
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

    return componentInfo;
}
