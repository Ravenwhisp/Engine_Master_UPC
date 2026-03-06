#include "Globals.h"
#include "Transform2D.h"

#include <imgui.h> 

Transform2D::Transform2D(UID id, GameObject* owner)
    : Component(id, ComponentType::TRANSFORM2D, owner)
{
}

std::unique_ptr<Component> Transform2D::clone(GameObject* newOwner) const
{
    std::unique_ptr<Transform2D> clonedComponent = std::make_unique<Transform2D>(m_uuid, newOwner);

    clonedComponent->position = position;
    clonedComponent->size = size;
    clonedComponent->pivot = pivot;
    clonedComponent->anchorMin = anchorMin;
    clonedComponent->anchorMax = anchorMax;

	return clonedComponent;
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

Rect2D Transform2D::getRect(const Rect2D& parent) const
{
    const float anchorMinX = std::max(0.0f, std::min(1.0f, anchorMin.x));
    const float anchorMinY = std::max(0.0f, std::min(1.0f, anchorMin.y));
    const float anchorMaxX = std::max(0.0f, std::min(1.0f, anchorMax.x));
    const float anchorMaxY = std::max(0.0f, std::min(1.0f, anchorMax.y));

    // anchor points in pixels inside parent
    const float anchorMinPixelX = parent.x + parent.w * anchorMinX;
    const float anchorMinPixelY = parent.y + parent.h * anchorMinY;
    const float anchorMaxPixelX = parent.x + parent.w * anchorMaxX;
    const float anchorMaxPixelY = parent.y + parent.h * anchorMaxY;

    float stretchW = (anchorMaxPixelX - anchorMinPixelX);
    float stretchH = (anchorMaxPixelY - anchorMinPixelY);

    if (stretchW < 0.0f)
    {
        stretchW = 0.0f;
    }
    if (stretchH < 0.0f)
    {
        stretchH = 0.0f;
    }

    // final size = stretch + size
    Rect2D rect;
    rect.w = std::max(0.0f, stretchW + size.x);
    rect.h = std::max(0.0f, stretchH + size.y);

    // reference position = anchorMin point + position
    const float referenceX = anchorMinPixelX + position.x;
    const float referenceY = anchorMinPixelY + position.y;

    rect.x = referenceX - pivot.x * rect.w;
    rect.y = referenceY - pivot.y * rect.h;

    return rect;
}

void Transform2D::drawUi()
{
    ImGui::Text("Transform2D");

    ImGui::SeparatorText("Anchor Presets");
    drawAnchorPresetsUI();

    ImGui::SeparatorText("Layout");
    ImGui::DragFloat2("Position", &position.x, 1.0f);
    ImGui::DragFloat2("Size", &size.x, 1.0f, -100000.0f, 100000.0f);

    ImGui::DragFloat2("Pivot", &pivot.x, 0.01f, 0.0f, 1.0f);

    ImGui::DragFloat2("Anchor Min", &anchorMin.x, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat2("Anchor Max", &anchorMax.x, 0.01f, 0.0f, 1.0f);

}

void Transform2D::setPointPreset(float ax, float ay, float px, float py)
{
    anchorMin = { ax, ay };
    anchorMax = { ax, ay };
    pivot = { px, py };
}

void Transform2D::drawAnchorPresetsUI()
{
    if (ImGui::Button("TopLeft"))
    {
        setPointPreset(0.0f, 0.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("TopRight"))
    {
        setPointPreset(1.0f, 0.0f, 1.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("BottomLeft"))
    {
        setPointPreset(0.0f, 1.0f, 0.0f, 1.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("BottomRight"))
    {
        setPointPreset(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (ImGui::Button("Center"))
    {
        setPointPreset(0.5f, 0.5f, 0.5f, 0.5f);
    }
    ImGui::SameLine();
    if (ImGui::Button("CenterTop"))
    {
        setPointPreset(0.5f, 0.0f, 0.5f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("CenterBottom")) {
        setPointPreset(0.5f, 1.0f, 0.5f, 1.0f);
    }
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

    {
        rapidjson::Value anchorMinData(rapidjson::kArrayType);
        anchorMinData.PushBack(anchorMin.x, domTree.GetAllocator());
        anchorMinData.PushBack(anchorMin.y, domTree.GetAllocator());
        componentInfo.AddMember("AnchorMin", anchorMinData, domTree.GetAllocator());
    }

    {
        rapidjson::Value anchorMaxData(rapidjson::kArrayType);
        anchorMaxData.PushBack(anchorMax.x, domTree.GetAllocator());
        anchorMaxData.PushBack(anchorMax.y, domTree.GetAllocator());
        componentInfo.AddMember("AnchorMax", anchorMaxData, domTree.GetAllocator());
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

    if (componentInfo.HasMember("AnchorMin"))
    {
        anchorMin.x = componentInfo["AnchorMin"][0].GetFloat();
        anchorMin.y = componentInfo["AnchorMin"][1].GetFloat();
    }

    if (componentInfo.HasMember("AnchorMax"))
    {
        anchorMax.x = componentInfo["AnchorMax"][0].GetFloat();
        anchorMax.y = componentInfo["AnchorMax"][1].GetFloat();
    }

    return true;
}