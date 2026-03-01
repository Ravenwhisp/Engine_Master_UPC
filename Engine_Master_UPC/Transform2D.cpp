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

rapidjson::Value Transform2D::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::TRANSFORM2D), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    {
        rapidjson::Value positionData(rapidjson::kArrayType);

        positionData.PushBack(position.x, domTree.GetAllocator());
        positionData.PushBack(position.y, domTree.GetAllocator());

        componentInfo.AddMember("Position", positionData, domTree.GetAllocator());
    }

    {
        rapidjson::Value sizeData(rapidjson::kArrayType);

        sizeData.PushBack(size.x, domTree.GetAllocator());
        sizeData.PushBack(size.y, domTree.GetAllocator());

        componentInfo.AddMember("Size", sizeData, domTree.GetAllocator());
    }

    {
        rapidjson::Value pivotData(rapidjson::kArrayType);
        pivotData.PushBack(pivot.x, domTree.GetAllocator());
        pivotData.PushBack(pivot.y, domTree.GetAllocator());
        componentInfo.AddMember("Pivot", pivotData, domTree.GetAllocator());
    }

    return componentInfo;
}

bool Transform2D::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("Position"))
    {
        position.x = componentInfo["Position"][0].GetFloat();
        position.y = componentInfo["Position"][1].GetFloat();     
    }

    if (componentInfo.HasMember("Size"))
    {
        size.x = componentInfo["Size"][0].GetFloat();
        size.y = componentInfo["Size"][1].GetFloat();
    }

    if (componentInfo.HasMember("Pivot"))
    {
        pivot.x = componentInfo["Pivot"][0].GetFloat();
        pivot.y = componentInfo["Pivot"][1].GetFloat();
    }

    return true;
}