#include "Globals.h"
#include "Transform2D.h"

#include <imgui.h> 

Transform2D::Transform2D(UID id, GameObject* owner)
    : Component(id, ComponentType::TRANSFORM2D, owner)
{
}

Rect2D Transform2D::getRect() const
{
    Rect2D r;
    r.w = size.x;
    r.h = size.y;

    r.x = position.x - pivot.x * r.w;
    r.y = position.y - pivot.y * r.h;

    return r;
}

void Transform2D::drawUi()
{
    ImGui::Text("Transform2D");
    ImGui::DragFloat2("Position", &position.x, 1.0f);
    ImGui::DragFloat2("Size", &size.x, 1.0f, 0.0f, 100000.0f);

    ImGui::DragFloat2("Pivot", &pivot.x, 0.01f, 0.0f, 1.0f);

    Rect2D r = getRect();
    ImGui::Text("Rect: x=%.1f y=%.1f w=%.1f h=%.1f", r.x, r.y, r.w, r.h);
}