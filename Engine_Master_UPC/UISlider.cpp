#include "Globals.h"
#include "UISlider.h"

#include <imgui.h>
#include "UIImage.h"
#include "SceneReferenceResolver.h"

UISlider::UISlider(UID id, GameObject* owner)
    : Component(id, ComponentType::UISLIDER, owner)
{
}

std::unique_ptr<Component> UISlider::clone(GameObject* newOwner) const
{
    std::unique_ptr<UISlider> clonedSlider = std::make_unique<UISlider>(m_uuid, newOwner);

    clonedSlider->setActive(this->isActive());
    clonedSlider->m_targetGraphic = this->m_targetGraphic;
    clonedSlider->m_targetGraphicUid = this->m_targetGraphicUid;

    clonedSlider->m_fillAmount = this->m_fillAmount;
    clonedSlider->m_fillMethod = this->m_fillMethod;
    clonedSlider->m_fillOrigin = this->m_fillOrigin;

    return clonedSlider;
}

void UISlider::applyToTarget()
{
    if (!m_targetGraphic)
    {
        return;
    }

    m_targetGraphic->setFillAmount(m_fillAmount);
    m_targetGraphic->setFillMethod(m_fillMethod);
    m_targetGraphic->setFillOrigin(m_fillOrigin);
}

void UISlider::setTargetGraphic(UIImage* img)
{
    m_targetGraphic = img;
    m_targetGraphicUid = img ? img->getID() : 0;
    applyToTarget();
}

void UISlider::setFillAmount(float amount)
{
    m_fillAmount = amount;
    applyToTarget();
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
    applyToTarget();
}

void UISlider::setFillOrigin(FillOrigin origin)
{
    m_fillOrigin = origin;
    applyToTarget();
}

void UISlider::drawUi()
{
    ImGui::Text("UISlider");

    ImGui::Button("Drop Here the Texture");

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
    ImGui::Text("Target Graphic: %s", m_targetGraphic ? "Assigned" : "None");

    ImGui::Separator();

    bool changed = false;
    changed |= ImGui::SliderFloat("Fill Amount", &m_fillAmount, 0.0f, 1.0f);

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
        applyToTarget();
    }
}

rapidjson::Value UISlider::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::UISLIDER), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("TargetGraphicUID", (uint64_t)m_targetGraphicUid, domTree.GetAllocator());

    componentInfo.AddMember("FillAmount", m_fillAmount, domTree.GetAllocator());
    componentInfo.AddMember("FillMethod", static_cast<int>(m_fillMethod), domTree.GetAllocator());
    componentInfo.AddMember("FillOrigin", static_cast<int>(m_fillOrigin), domTree.GetAllocator());

    return componentInfo;
}

bool UISlider::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TargetGraphicUID"))
    {
        m_targetGraphicUid = (UID)componentInfo["TargetGraphicUID"].GetUint64();
    }

    if (componentInfo.HasMember("FillAmount"))
        m_fillAmount = componentInfo["FillAmount"].GetFloat();

    if (componentInfo.HasMember("FillMethod"))
        m_fillMethod = static_cast<FillMethod>(componentInfo["FillMethod"].GetInt());

    if (componentInfo.HasMember("FillOrigin"))
    {
        m_fillOrigin = static_cast<FillOrigin>(componentInfo["FillOrigin"].GetInt());
    }
    else if (componentInfo.HasMember("Clockwise"))
    {
        const auto& clockwiseValue = componentInfo["Clockwise"];
        if (clockwiseValue.IsBool())
        {
            m_fillOrigin = clockwiseValue.GetBool() ? FillOrigin::Radial360Clockwise : FillOrigin::Radial360CounterClockwise;
        }
        else if (clockwiseValue.IsInt())
        {
            m_fillOrigin = static_cast<FillOrigin>(clockwiseValue.GetInt());
        }
    }

    m_targetGraphic = nullptr;
    return true;
}

void UISlider::fixReferences(const SceneReferenceResolver& resolver)
{
    if (m_targetGraphicUid == 0)
    {
        m_targetGraphic = nullptr;
        return;
    }

    m_targetGraphic = static_cast<UIImage*>(resolver.getClonedComponent(m_targetGraphicUid));
    applyToTarget();
}
