#include "Globals.h"
#include "Canvas.h"

#include <imgui.h>

Canvas::Canvas(UID id, GameObject* owner)
    : Component(id, ComponentType::CANVAS, owner)
{
}

void Canvas::drawUi()
{
    ImGui::Text("Canvas");
    ImGui::Checkbox("Screen Space", &isScreenSpace);
}

