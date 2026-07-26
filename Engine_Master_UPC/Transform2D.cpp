#include "Globals.h"
#include "Transform2D.h"
#include "JsonArchive.h"

#include <imgui.h> 

Transform2D::Transform2D(UID id, GameObject* owner)
    : Component(id, ComponentType::TRANSFORM2D, owner)
{
}

std::unique_ptr<Component> Transform2D::clone(GameObject* newOwner) const
{
    std::unique_ptr<Transform2D> clonedComponent = std::make_unique<Transform2D>(m_uuid, newOwner);

    clonedComponent->position = position;
    clonedComponent->baseSize = baseSize;
    clonedComponent->scale = scale;
    clonedComponent->pivot = pivot;
    clonedComponent->anchorMin = anchorMin;
    clonedComponent->anchorMax = anchorMax;
    clonedComponent->stretchMode = stretchMode;
    clonedComponent->sizingMode = sizingMode;
    clonedComponent->aspectRatio = aspectRatio;
    clonedComponent->alpha = alpha;

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

Rect2D Transform2D::getRect(const Rect2D& parent, const Vector2& inheritedScale) const
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
    const Vector2 effectiveScale = { scale.x * inheritedScale.x, scale.y * inheritedScale.y };
    const Vector2 scaledPosition = { position.x * inheritedScale.x, position.y * inheritedScale.y };
    
    if (stretchMode == StretchMode::NONE)
    {
        if (sizingMode == SizingMode::KEEP_ASPECT_RATIO)
        {
            rect.w = baseSize.x * effectiveScale.x;
            rect.h = rect.w / baseAspectRatio;
        }
        else
        {
            rect.w = baseSize.x * effectiveScale.x;
            rect.h = baseSize.y * effectiveScale.y;
        }
        referenceX = anchorMinPixelX + scaledPosition.x;
        referenceY = anchorMinPixelY + scaledPosition.y;
    }
    else if (stretchMode == StretchMode::BOTH)
    {
        rect.w = std::max(0.0f, stretchW * scale.x);
        rect.h = std::max(0.0f, stretchH * scale.y);

        float centerX = (anchorMinPixelX + anchorMaxPixelX) * 0.5f;
        float centerY = (anchorMinPixelY + anchorMaxPixelY) * 0.5f;

        referenceX = centerX + scaledPosition.x;
        referenceY = centerY + scaledPosition.y;
    }
    else if (stretchMode == StretchMode::HORIZONTAL)
    {
        rect.w = std::max(0.0f, stretchW);
        rect.h = rect.w / baseAspectRatio;

        if (sizingMode == SizingMode::FIXED_SIZE)
        {
            rect.h *= effectiveScale.y;
        }

        float centerX = (anchorMinPixelX + anchorMaxPixelX) * 0.5f;

        referenceX = centerX + scaledPosition.x;
        referenceY = anchorMinPixelY + scaledPosition.y;
    }
    else if (stretchMode == StretchMode::VERTICAL)
    {
        rect.h = std::max(0.0f, stretchH);
        rect.w = rect.h * baseAspectRatio;

        if (sizingMode == SizingMode::FIXED_SIZE)
        {
            rect.w *= effectiveScale.x;
        }

        float centerY = (anchorMinPixelY + anchorMaxPixelY) * 0.5f;

        referenceX = anchorMinPixelX + scaledPosition.x;
        referenceY = centerY + scaledPosition.y;
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
    ImGui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
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

void Transform2D::serialize(IArchive& archive)
{
    Component::serialize(archive);

    {
        DirectX::SimpleMath::Vector3 pos(position.x, position.y, 0.0f);
        archive.serialize(pos, "Position");
        if (archive.mode() == ArchiveMode::Input)
        {
            position.x = pos.x;
            position.y = pos.y;
        }
    }

    {
        DirectX::SimpleMath::Vector3 s(scale.x, scale.y, 0.0f);
        archive.serialize(s, "Scale");
        if (archive.mode() == ArchiveMode::Input)
        {
            scale.x = s.x;
            scale.y = s.y;
        }
    }

    {
        DirectX::SimpleMath::Vector3 piv(pivot.x, pivot.y, 0.0f);
        archive.serialize(piv, "Pivot");
        if (archive.mode() == ArchiveMode::Input)
        {
            pivot.x = piv.x;
            pivot.y = piv.y;
        }
    }

    {
        DirectX::SimpleMath::Vector3 aMin(anchorMin.x, anchorMin.y, 0.0f);
        archive.serialize(aMin, "AnchorMin");
        if (archive.mode() == ArchiveMode::Input)
        {
            anchorMin.x = aMin.x;
            anchorMin.y = aMin.y;
        }
    }

    {
        DirectX::SimpleMath::Vector3 aMax(anchorMax.x, anchorMax.y, 0.0f);
        archive.serialize(aMax, "AnchorMax");
        if (archive.mode() == ArchiveMode::Input)
        {
            anchorMax.x = aMax.x;
            anchorMax.y = aMax.y;
        }
    }

    {
        DirectX::SimpleMath::Vector3 bSize(baseSize.x, baseSize.y, 0.0f);
        archive.serialize(bSize, "BaseSize");
        if (archive.mode() == ArchiveMode::Input)
        {
            baseSize.x = bSize.x;
            baseSize.y = bSize.y;
        }
    }

    archive.serializeStringEnum(stretchMode, "StretchMode", StretchModeToString, StringToStretchMode);

    archive.serializeStringEnum(sizingMode, "SizingMode", SizingModeToString, StringToSizingMode);

    archive.serialize(aspectRatio, "AspectRatio");
    archive.serialize(alpha, "Alpha");
}