#include "Globals.h"
#include "UISlider.h"

#include <imgui.h>
#include "UIImage.h"
#include "GameObject.h"
#include "Transform2D.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "UILayoutUtils.h"

#define M_PI 3.14159265358979323846f

UISlider::UISlider(UID id, GameObject* owner)
    : Component(id, ComponentType::UISLIDER, owner)
{
}

std::unique_ptr<Component> UISlider::clone(GameObject* newOwner) const
{
    std::unique_ptr<UISlider> clonedSlider = std::make_unique<UISlider>(m_uuid, newOwner);

    clonedSlider->setActive(this->isActive());

    clonedSlider->m_fillAmount = this->m_fillAmount;
    clonedSlider->m_fillMethod = this->m_fillMethod;
    clonedSlider->m_fillOrigin = this->m_fillOrigin;

    return clonedSlider;
}

void UISlider::applyToImage()
{
    if (!getOwner())
    {
        return;
    }

    UIImage* img = getOwner()->GetComponentAs<UIImage>(ComponentType::UIIMAGE);
    
    if (!img)
    {
        return;
    }

    img->setFillAmount(m_fillAmount);
    img->setFillMethod(m_fillMethod);
    img->setFillOrigin(m_fillOrigin);
}

void UISlider::setFillAmount(const Vector2& amount)
{
    m_fillAmount = amount;
    applyToImage();
}

void UISlider::setFillStart(float start)
{
    m_fillAmount.x = start;
    applyToImage();
}

void UISlider::setFillEnd(float end)
{
    m_fillAmount.y = end;
    applyToImage();
}

void UISlider::onPointerEnter(PointerEventData& data)
{
    //solo para texturas
}

void UISlider::onPointerExit(PointerEventData& data)
{
    //solo para texturas
}

void UISlider::onPointerDown(PointerEventData& data)
{
    if (!isActive()) return;

    updateFillAmountFromPointerPosition(data.position);
}

void UISlider::onPointerDrag(PointerEventData& data)
{
    if (!isActive()) return;

    updateFillAmountFromPointerPosition(data.position);
}

void UISlider::onPointerUp(PointerEventData& data)
{
    //solo para texturas
}

void UISlider::onPointerClick(PointerEventData& data)
{
    updateFillAmountFromPointerPosition(data.position);
}

void UISlider::updateFillAmountFromPointerPosition(const Vector2& mousePos)
{
    if (!getOwner()) return;

    Transform2D* rootTransform = getOwner()->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);
    if (!rootTransform) return;

#ifdef GAME_RELEASE
    auto viewport = app->getModuleD3D12()->getSwapChain()->getViewport();
    Vector2 size(viewport.Width, viewport.Height);
#else
    auto size = app->getModuleEditor()->getEventViewportSize();
#endif

    Vector2 uiScale(1.0f, 1.0f);
    uiScale = UILayoutUtils::CalculateScreenSpaceScale(size.x, size.y);

    Vector2 virtualMousePos(0.0f, 0.0f);
    if (uiScale.x > 0.0f) virtualMousePos.x = mousePos.x / uiScale.x;
    if (uiScale.y > 0.0f) virtualMousePos.y = mousePos.y / uiScale.y;

    Vector2 lonaVirtual = Vector2(size.x, size.y) / uiScale;
    Vector2 position = rootTransform->getPosition();
    Vector2 baseSize = rootTransform->getBaseSize();
    Vector2 scale = rootTransform->getScale();
    Vector2 pivot = rootTransform->getPivot();

    Vector2 anchorMin = rootTransform->getAnchorMin();
    Vector2 anchorMax = rootTransform->getAnchorMax();
    StretchMode stretchMode = rootTransform->getStretchMode();

    float anchorMinPixelX = lonaVirtual.x * std::max(0.0f, std::min(1.0f, anchorMin.x));
    float anchorMinPixelY = lonaVirtual.y * std::max(0.0f, std::min(1.0f, anchorMin.y));
    float anchorMaxPixelX = lonaVirtual.x * std::max(0.0f, std::min(1.0f, anchorMax.x));
    float anchorMaxPixelY = lonaVirtual.y * std::max(0.0f, std::min(1.0f, anchorMax.y));

    float stretchW = std::max(0.0f, anchorMaxPixelX - anchorMinPixelX);
    float stretchH = std::max(0.0f, anchorMaxPixelY - anchorMinPixelY);

    float width = baseSize.x * scale.x;
    float height = baseSize.y * scale.y;
    float referenceX = anchorMinPixelX + position.x;
    float referenceY = anchorMinPixelY + position.y;

    if (stretchMode == StretchMode::BOTH)
    {
        width = stretchW * scale.x;
        height = stretchH * scale.y;
        referenceX = ((anchorMinPixelX + anchorMaxPixelX) * 0.5f) + position.x;
        referenceY = ((anchorMinPixelY + anchorMaxPixelY) * 0.5f) + position.y;
    }
    else if (stretchMode == StretchMode::HORIZONTAL)
    {
        width = stretchW;

        float baseAspectRatio = (baseSize.y > 0.0f) ? (baseSize.x / baseSize.y) : 1.0f;
        height = width / baseAspectRatio;

        referenceX = ((anchorMinPixelX + anchorMaxPixelX) * 0.5f) + position.x;
    }
    else if (stretchMode == StretchMode::VERTICAL)
    {
        height = stretchH;
        float baseAspectRatio = (baseSize.y > 0.0f) ? (baseSize.x / baseSize.y) : 1.0f;
        width = height * baseAspectRatio;

        referenceY = ((anchorMinPixelY + anchorMaxPixelY) * 0.5f) + position.y;
    }

    float globalPosX = referenceX - (pivot.x * width);
    float globalPosY = referenceY - (pivot.y * height);

    float newPercentage = 0.0f;

    switch (m_fillMethod)
    {
    case FillMethod::Horizontal:
    {
        float minX = globalPosX;
        if (width > 0.0f)
        {
            newPercentage = (virtualMousePos.x - minX) / width;
        }
        if (m_fillOrigin == FillOrigin::HorizontalRight)
        {
            newPercentage = 1.0f - newPercentage;
        }
        break;
    }

    case FillMethod::Vertical:
    {
        float minY = globalPosY;
        if (height > 0.0f)
        {
            newPercentage = 1.0f - ((virtualMousePos.y - minY) / height);
        }
        if (m_fillOrigin == FillOrigin::VerticalTop)
        {
            newPercentage = 1.0f - newPercentage;
        }
        break;
    }

    case FillMethod::Radial90:
    case FillMethod::Radial180:
    case FillMethod::Radial360:
    {
        Vector2 centroGeometrico = Vector2(globalPosX + (width / 2.0f), globalPosY + (height / 2.0f));
        Vector2 dir = virtualMousePos - centroGeometrico;

        if (dir.LengthSquared() < 0.001f) {
            newPercentage = m_fillAmount.y;
            break;
        }

        float mouseAngle = atan2f(-dir.y, dir.x);
        if (mouseAngle < 0.0f) mouseAngle += 2.0f * M_PI;

        float startAngleOffset = M_PI / 2.0f;
        float maxApertureAngle = 2.0f * M_PI;

        if (m_fillMethod == FillMethod::Radial180)
        {
            maxApertureAngle = M_PI;
        }
        else if (m_fillMethod == FillMethod::Radial90)
        {
            maxApertureAngle = M_PI / 2.0f;
        }

        bool clockwise = true;
        if (m_fillMethod == FillMethod::Radial360)
        {
            clockwise = (m_fillOrigin != FillOrigin::Radial360CounterClockwise);
        }
        else
        {
            clockwise = (static_cast<int>(m_fillOrigin) & 4) == 0;
        }

        float relativeAngle = 0.0f;
        if (clockwise)
        {
            relativeAngle = startAngleOffset - mouseAngle;
        }
        else
        {
            relativeAngle = mouseAngle - startAngleOffset;
        }

        if (relativeAngle < 0.0f) relativeAngle += 2.0f * M_PI;
        if (maxApertureAngle > 0.0f)
        {
            newPercentage = relativeAngle / maxApertureAngle;
        }
        if (m_fillMethod != FillMethod::Radial360 && newPercentage > 1.0f)
        {
            newPercentage = m_fillAmount.y;
        }

        break;
    }
    }

    newPercentage = std::clamp(newPercentage, 0.0f, 1.0f);
    m_fillAmount.y = newPercentage;

    applyToImage();
}

void UISlider::setFillMethod(FillMethod method)
{
    m_fillMethod = method;
    switch (m_fillMethod)
    {
    case FillMethod::Horizontal:
        m_fillOrigin = FillOrigin::HorizontalLeft;
        break;
    case FillMethod::Vertical:
        m_fillOrigin = FillOrigin::VerticalBottom;
        break;
    case FillMethod::Radial90:
        m_fillOrigin = FillOrigin::Radial90BottomLeft;
        break;
    case FillMethod::Radial180:
        m_fillOrigin = FillOrigin::Radial180Bottom;
        break;
    case FillMethod::Radial360:
        m_fillOrigin = FillOrigin::Radial360Clockwise;
        break;
    }
    applyToImage();
}

void UISlider::setFillOrigin(FillOrigin origin)
{
    m_fillOrigin = origin;
    applyToImage();
}

void UISlider::drawUi()
{
    ImGui::Text("UISlider");

    ImGui::Separator();

    bool changed = false;
    // Allow editing start and end separately
    float start = m_fillAmount.x;
    float end = m_fillAmount.y;
    if (ImGui::SliderFloat("Fill Start", &start, 0.0f, 1.0f))
    {
        m_fillAmount.x = start;
        changed = true;
    }
    if (ImGui::SliderFloat("Fill End", &end, 0.0f, 1.0f))
    {
        m_fillAmount.y = end;
        changed = true;
    }

    const char* fillMethods[] = { "Horizontal", "Vertical", "Radial 90", "Radial 180", "Radial 360" };
    int currentMethod = static_cast<int>(m_fillMethod);
    if (ImGui::Combo("Fill Method", &currentMethod, fillMethods, IM_ARRAYSIZE(fillMethods)))
    {
        setFillMethod(static_cast<FillMethod>(currentMethod));
        changed = true;
    }

    if (m_fillMethod == FillMethod::Horizontal)
    {
        const char* horizontalOptions[] = { "Left to Right", "Right to Left" };
        int origin = (m_fillOrigin == FillOrigin::HorizontalRight) ? 1 : 0;
        if (ImGui::Combo("Direction", &origin, horizontalOptions, IM_ARRAYSIZE(horizontalOptions)))
        {
            m_fillOrigin = origin == 0 ? FillOrigin::HorizontalLeft : FillOrigin::HorizontalRight;
            changed = true;
        }
    }
    else if (m_fillMethod == FillMethod::Vertical)
    {
        const char* verticalOptions[] = { "Bottom to Top", "Top to Bottom" };
        int origin = (m_fillOrigin == FillOrigin::VerticalTop) ? 1 : 0;
        if (ImGui::Combo("Direction", &origin, verticalOptions, IM_ARRAYSIZE(verticalOptions)))
        {
            m_fillOrigin = origin == 0 ? FillOrigin::VerticalBottom : FillOrigin::VerticalTop;
            changed = true;
        }
    }
    else if (m_fillMethod == FillMethod::Radial90)
    {
        const char* radial90Options[] = { "Bottom Left", "Top Left", "Top Right", "Bottom Right" };
        int origin = static_cast<int>(m_fillOrigin) & 3;
        bool clockwise = (static_cast<int>(m_fillOrigin) & 4) == 0;
        if (ImGui::Combo("Corner", &origin, radial90Options, IM_ARRAYSIZE(radial90Options)))
        {
            m_fillOrigin = static_cast<FillOrigin>((clockwise ? 0 : 4) + origin);
            changed = true;
        }
        if (ImGui::Checkbox("Clockwise", &clockwise))
        {
            m_fillOrigin = static_cast<FillOrigin>((clockwise ? 0 : 4) + origin);
            changed = true;
        }
    }
    else if (m_fillMethod == FillMethod::Radial180)
    {
        const char* radial180Options[] = { "Bottom", "Left", "Top", "Right" };
        int origin = static_cast<int>(m_fillOrigin) & 3;
        bool clockwise = (static_cast<int>(m_fillOrigin) & 4) == 0;
        if (ImGui::Combo("Side", &origin, radial180Options, IM_ARRAYSIZE(radial180Options)))
        {
            m_fillOrigin = static_cast<FillOrigin>((clockwise ? 0 : 4) + origin);
            changed = true;
        }
        if (ImGui::Checkbox("Clockwise", &clockwise))
        {
            m_fillOrigin = static_cast<FillOrigin>((clockwise ? 0 : 4) + origin);
            changed = true;
        }
    }
    else if (m_fillMethod == FillMethod::Radial360)
    {
        bool clockwise = (m_fillOrigin != FillOrigin::Radial360CounterClockwise);
        if (ImGui::Checkbox("Clockwise", &clockwise))
        {
            m_fillOrigin = clockwise ? FillOrigin::Radial360Clockwise : FillOrigin::Radial360CounterClockwise;
            changed = true;
        }
    }

    if (changed)
    {
        applyToImage();
    }
}

void UISlider::serialize(IArchive& archive)
{
	Component::serialize(archive);

	{
		uint32_t count = 2;
		archive.beginArray(count, "FillAmount");
		float start = m_fillAmount.x;
		float end = m_fillAmount.y;
		archive.serialize(start, "");
		archive.serialize(end, "");
		archive.endArray();
		if (archive.mode() == ArchiveMode::Input)
		{
			m_fillAmount.x = start;
			m_fillAmount.y = end;
		}
	}

	archive.serializeStringEnum(m_fillMethod, "FillMethod", FillMethodToString, StringToFillMethod);
	archive.serialize(m_fillOrigin, "FillOrigin");
}
