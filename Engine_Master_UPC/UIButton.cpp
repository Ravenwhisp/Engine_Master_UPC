#include "Globals.h"
#include "UIButton.h"
#include "UIImage.h"
#include <imgui.h>

UIButton::UIButton(UID id, GameObject* owner)
    : Component(id, ComponentType::UIBUTTON, owner)
{
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