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
    clonedSlider->m_clockwise = this->m_clockwise;

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
    m_targetGraphic->setClockwise(m_clockwise);
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
    applyToTarget();
}

void UISlider::setClockwise(bool clockwise)
{
    m_clockwise = clockwise;
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
        m_fillMethod = static_cast<FillMethod>(currentMethod);
        changed = true;
    }

    if (m_fillMethod >= FillMethod::Radial90)
    {
        changed |= ImGui::Checkbox("Clockwise", &m_clockwise);
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
    componentInfo.AddMember("Clockwise", m_clockwise, domTree.GetAllocator());

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

    if (componentInfo.HasMember("Clockwise"))
        m_clockwise = componentInfo["Clockwise"].GetBool();

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
