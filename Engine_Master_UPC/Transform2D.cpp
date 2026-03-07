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
    
    if (sizingMode == SizingMode::KEEP_ASPECT_RATIO)
    {
        float baseAspectRatio = (baseSize.y > 0.0f) ? (baseSize.x / baseSize.y) : 1.0f;
        r.w = baseSize.x * scale.x;
        r.h = r.w / baseAspectRatio;
    }
    else
    {
        r.w = baseSize.x * scale.x;
        r.h = baseSize.y * scale.y;
    }

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

    const float anchorMinPixelX = parent.x + parent.w * anchorMinX;
    const float anchorMinPixelY = parent.y + parent.h * anchorMinY;
    const float anchorMaxPixelX = parent.x + parent.w * anchorMaxX;
    const float anchorMaxPixelY = parent.y + parent.h * anchorMaxY;

    float stretchW = (anchorMaxPixelX - anchorMinPixelX);
    float stretchH = (anchorMaxPixelY - anchorMinPixelY);

    if (stretchW < 0.0f) stretchW = 0.0f;
    if (stretchH < 0.0f) stretchH = 0.0f;

    Rect2D rect;
    float referenceX = 0.0f;
    float referenceY = 0.0f;
    
    float baseAspectRatio = (baseSize.y > 0.0f) ? (baseSize.x / baseSize.y) : 1.0f;
    
    if (stretchMode == StretchMode::NONE)
    {
        if (sizingMode == SizingMode::KEEP_ASPECT_RATIO)
        {
            rect.w = baseSize.x * scale.x;
            rect.h = rect.w / baseAspectRatio;
        }
        else
        {
            rect.w = baseSize.x * scale.x;
            rect.h = baseSize.y * scale.y;
        }
        
        referenceX = anchorMinPixelX + position.x;
        referenceY = anchorMinPixelY + position.y;
    }
    else if (stretchMode == StretchMode::BOTH)
    {
        rect.w = std::max(0.0f, stretchW);
        rect.h = std::max(0.0f, stretchH);
        
        float centerX = (anchorMinPixelX + anchorMaxPixelX) * 0.5f;
        float centerY = (anchorMinPixelY + anchorMaxPixelY) * 0.5f;
        
        referenceX = centerX + position.x;
        referenceY = centerY + position.y;
    }
    else if (stretchMode == StretchMode::HORIZONTAL)
    {
        rect.w = std::max(0.0f, stretchW);
        rect.h = rect.w / baseAspectRatio;
        
        if (sizingMode == SizingMode::FIXED_SIZE)
        {
            rect.h *= scale.y;
        }
        
        float centerX = (anchorMinPixelX + anchorMaxPixelX) * 0.5f;
        
        referenceX = centerX + position.x;
        referenceY = anchorMinPixelY + position.y;
    }
    else if (stretchMode == StretchMode::VERTICAL)
    {
        rect.h = std::max(0.0f, stretchH);
        rect.w = rect.h * baseAspectRatio;
        
        if (sizingMode == SizingMode::FIXED_SIZE)
        {
            rect.w *= scale.x;
        }
        
        float centerY = (anchorMinPixelY + anchorMaxPixelY) * 0.5f;
        
        referenceX = anchorMinPixelX + position.x;
        referenceY = centerY + position.y;
    }

    rect.x = referenceX - pivot.x * rect.w;
    rect.y = referenceY - pivot.y * rect.h;

    return rect;
}

void Transform2D::setStretchMode(StretchMode mode)
{
    stretchMode = mode;
    
    if (mode == StretchMode::NONE)
    {
        anchorMax = anchorMin;
    }
}

void Transform2D::drawUi()
{
    ImGui::Text("Transform2D");

    ImGui::DragFloat2("Position", &position.x, 1.0f);
    ImGui::DragFloat2("Pivot", &pivot.x, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat2("Base Size", &baseSize.x, 1.0f, 0.0f, 10000.0f);
    
    ImGui::SeparatorText("Stretch Presets");
    drawStretchPresetsUI();

    ImGui::SeparatorText("Layout");
    
    const char* stretchModeNames[] = { "None", "Horizontal", "Vertical", "Both" };
    int currentStretchMode = static_cast<int>(stretchMode);
    if (ImGui::Combo("Stretch Mode", &currentStretchMode, stretchModeNames, 4))
    {
        setStretchMode(static_cast<StretchMode>(currentStretchMode));
    }
    
    if (stretchMode == StretchMode::HORIZONTAL || stretchMode == StretchMode::VERTICAL || stretchMode == StretchMode::NONE)
    {
        bool keepAspect = (sizingMode == SizingMode::KEEP_ASPECT_RATIO);
        if (ImGui::Checkbox("Keep Aspect Ratio", &keepAspect))
        {
            sizingMode = keepAspect ? SizingMode::KEEP_ASPECT_RATIO : SizingMode::FIXED_SIZE;
        }
        
        if (stretchMode == StretchMode::NONE)
        {
            if (sizingMode == SizingMode::KEEP_ASPECT_RATIO)
            {
                ImGui::DragFloat("Scale", &scale.x, 0.01f, 0.0f, 10.0f);
                scale.y = scale.x;
            }
            else
            {
                ImGui::DragFloat2("Scale", &scale.x, 0.01f, 0.0f, 10.0f);
            }
        }
        else if (sizingMode == SizingMode::FIXED_SIZE)
        {
            if (stretchMode == StretchMode::HORIZONTAL)
            {
                ImGui::DragFloat("Height Scale", &scale.y, 0.01f, 0.0f, 10.0f);
            }
            else if (stretchMode == StretchMode::VERTICAL)
            {
                ImGui::DragFloat("Width Scale", &scale.x, 0.01f, 0.0f, 10.0f);
            }
        }
    }
    else
    {
        ImGui::DragFloat2("Scale", &scale.x, 0.01f, 0.0f, 10.0f);
    }

    ImGui::SeparatorText("Anchor Presets");
    drawAnchorPresetsUI();

    if (stretchMode != StretchMode::NONE)
    {
        ImGui::DragFloat2("Anchor Min", &anchorMin.x, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat2("Anchor Max", &anchorMax.x, 0.01f, 0.0f, 1.0f);
    }
    else
    {
        ImGui::DragFloat2("Anchor", &anchorMin.x, 0.01f, 0.0f, 1.0f);
        anchorMax = anchorMin;
    }
}

void Transform2D::setPointPreset(float ax, float ay, float px, float py)
{
    anchorMin = { ax, ay };
    anchorMax = { ax, ay };
    pivot = { px, py };
    stretchMode = StretchMode::NONE;
    position = { 0.0f, 0.0f };
    scale = { 1.0f, 1.0f };
    sizingMode = SizingMode::KEEP_ASPECT_RATIO;
}

void Transform2D::setStretchPreset(StretchMode mode, float axMin, float ayMin, float axMax, float ayMax, float px, float py)
{
    stretchMode = mode;
    anchorMin = { axMin, ayMin };
    anchorMax = { axMax, ayMax };
    pivot = { px, py };
    position = { 0.0f, 0.0f };
    scale = { 1.0f, 1.0f };
    
    if (mode == StretchMode::HORIZONTAL || mode == StretchMode::VERTICAL)
    {
        sizingMode = SizingMode::KEEP_ASPECT_RATIO;
    }
}

void Transform2D::drawStretchPresetsUI()
{
    float buttonSize = 40.0f;
    ImVec2 btnSize(buttonSize, buttonSize);
    
    // Row 1: Horizontal stretches
    if (ImGui::Button("H-T##HT", btnSize)) { setStretchPreset(StretchMode::HORIZONTAL, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Horizontal Top");
    ImGui::SameLine();
    if (ImGui::Button("H-C##HC", btnSize)) { setStretchPreset(StretchMode::HORIZONTAL, 0.0f, 0.5f, 1.0f, 0.5f, 0.5f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Horizontal Center");
    ImGui::SameLine();
    if (ImGui::Button("H-B##HB", btnSize)) { setStretchPreset(StretchMode::HORIZONTAL, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f, 1.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Horizontal Bottom");
    
    // Row 2: Vertical stretches and full stretch
    if (ImGui::Button("V-L##VL", btnSize)) { setStretchPreset(StretchMode::VERTICAL, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Vertical Left");
    ImGui::SameLine();
    if (ImGui::Button("V-C##VC", btnSize)) { setStretchPreset(StretchMode::VERTICAL, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Both");
    ImGui::SameLine();
    if (ImGui::Button("V-R##VR", btnSize)) { setStretchPreset(StretchMode::VERTICAL, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Vertical Right");

    // Row 2: Full stretch
    if (ImGui::Button("ALL##BOTH", btnSize)) { setStretchPreset(StretchMode::BOTH, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stretch Both");
}

void Transform2D::drawAnchorPresetsUI()
{
    float buttonSize = 40.0f;
    ImVec2 btnSize(buttonSize, buttonSize);
    
    // Row 1: Top anchors
    if (ImGui::Button("TL##TL", btnSize)) { setPointPreset(0.0f, 0.0f, 0.0f, 0.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Top Left");
    ImGui::SameLine();
    if (ImGui::Button("TC##TC", btnSize)) { setPointPreset(0.5f, 0.0f, 0.5f, 0.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Top Center");
    ImGui::SameLine();
    if (ImGui::Button("TR##TR", btnSize)) { setPointPreset(1.0f, 0.0f, 1.0f, 0.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Top Right");
    
    // Row 2: Middle anchors
    if (ImGui::Button("ML##ML", btnSize)) { setPointPreset(0.0f, 0.5f, 0.0f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center Left");
    ImGui::SameLine();
    if (ImGui::Button("CC##MC", btnSize)) { setPointPreset(0.5f, 0.5f, 0.5f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center");
    ImGui::SameLine();
    if (ImGui::Button("MR##MR", btnSize)) { setPointPreset(1.0f, 0.5f, 1.0f, 0.5f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center Right");
    
    // Row 3: Bottom anchors
    if (ImGui::Button("BL##BL", btnSize)) { setPointPreset(0.0f, 1.0f, 0.0f, 1.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bottom Left");
    ImGui::SameLine();
    if (ImGui::Button("BC##BC", btnSize)) { setPointPreset(0.5f, 1.0f, 0.5f, 1.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bottom Center");
    ImGui::SameLine();
    if (ImGui::Button("BR##BR", btnSize)) { setPointPreset(1.0f, 1.0f, 1.0f, 1.0f); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bottom Right");
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
        rapidjson::Value scaleData(rapidjson::kArrayType);
        scaleData.PushBack(scale.x, domTree.GetAllocator());
        scaleData.PushBack(scale.y, domTree.GetAllocator());
        componentInfo.AddMember("Scale", scaleData, domTree.GetAllocator());
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
    
    {
        rapidjson::Value baseSizeData(rapidjson::kArrayType);
        baseSizeData.PushBack(baseSize.x, domTree.GetAllocator());
        baseSizeData.PushBack(baseSize.y, domTree.GetAllocator());
        componentInfo.AddMember("BaseSize", baseSizeData, domTree.GetAllocator());
    }
    
    componentInfo.AddMember("StretchMode", static_cast<int>(stretchMode), domTree.GetAllocator());
    componentInfo.AddMember("SizingMode", static_cast<int>(sizingMode), domTree.GetAllocator());
    componentInfo.AddMember("AspectRatio", aspectRatio, domTree.GetAllocator());

    return componentInfo;
}

bool Transform2D::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("Position"))
    {
        position.x = componentInfo["Position"][0].GetFloat();
        position.y = componentInfo["Position"][1].GetFloat();     
    }

    if (componentInfo.HasMember("Scale"))
    {
        scale.x = componentInfo["Scale"][0].GetFloat();
        scale.y = componentInfo["Scale"][1].GetFloat();
    }
    else if (componentInfo.HasMember("Size"))
    {
        scale.x = componentInfo["Size"][0].GetFloat() / baseSize.x;
        scale.y = componentInfo["Size"][1].GetFloat() / baseSize.y;
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
    
    if (componentInfo.HasMember("BaseSize"))
    {
        baseSize.x = componentInfo["BaseSize"][0].GetFloat();
        baseSize.y = componentInfo["BaseSize"][1].GetFloat();
    }
    
    if (componentInfo.HasMember("StretchMode"))
    {
        stretchMode = static_cast<StretchMode>(componentInfo["StretchMode"].GetInt());
    }
    
    if (componentInfo.HasMember("SizingMode"))
    {
        int loadedMode = componentInfo["SizingMode"].GetInt();
        sizingMode = static_cast<SizingMode>(loadedMode);
    }
    else
    {
        sizingMode = SizingMode::KEEP_ASPECT_RATIO;
    }
    
    if (componentInfo.HasMember("AspectRatio"))
    {
        aspectRatio = componentInfo["AspectRatio"].GetFloat();
    }

    return true;
}