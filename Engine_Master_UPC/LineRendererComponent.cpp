#include "Globals.h"
#include "LineRendererComponent.h"
#include "JsonArchive.h"

#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "ModuleInput.h"
#include "imgui_bezier.h"
#include "imgui_color_gradient.h"
#include "ModuleTime.h"


LineRendererComponent::LineRendererComponent(UID id, GameObject* owner) : Component(id, ComponentType::TRAIL, owner)
{
    //Create two points to start
    CreatePoint();
    CreatePoint();

    m_color.clearMarks(); // because it has default values that we don't want

    m_color.addMark(0.f, ImColor(1.f, 1.f, 1.f, 1.f));
    m_color.addAlphaMark(0.f, 1.0f);
    m_color.addMark(1.f, ImColor(1.f, 1.f, 1.f, 1.f));
    m_color.addAlphaMark(1.f, 1.0f);
    m_color.setEditAlpha(true);
}

void LineRendererComponent::drawUi()
{
    ImGui::Text("Line Renderer");

    // LINE RENDERER INTERFACE

    // point controller //

    //TODO: Edit point list, adding and removing points and editing its values
    //      Be able to attach gameobjects to each point.


    /*ImGui::Button("TargetGraphic reference");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
        {
            Component* comp = *(Component**)payload->Data;

            if (comp && comp->getType() == ComponentType::TRANSFORM)
            {
                setTargetGraphic(static_cast<UIImage*>(comp));
            }
        }
        ImGui::EndDragDropTarget();
    }*/
    

    // Color gradient editor

    if (ImGui::GradientButton(&m_color)) {

        ImGui::OpenPopup("GradientEditorPopup");
    }

    ImGui::SameLine(); ImGui::Text("Color Gradient");


    if (ImGui::BeginPopup("GradientEditorPopup")) {

        ImGui::GradientEditor(&m_color, m_draggingMark, m_selectedMark);

        if (app->getModuleInput()->isKeyJustPressed(Keyboard::Keys::Delete) && m_selectedMark != nullptr)
        {
            m_color.removeMark(m_selectedMark);
            m_selectedMark = nullptr;
            m_draggingMark = nullptr;
        }

        ImGui::EndPopup();
    }

    
}

void LineRendererComponent::update()
{
   
    for (auto point = m_points.begin(); point != m_points.end(); )
    {
        if (point->get()->transformParent != nullptr)
        {
            const Matrix& globalMatrix = point->get()->transformParent->getGlobalMatrix();
            point->get()->position = globalMatrix.Translation();
            point->get()->rotation = Quaternion::CreateFromRotationMatrix(globalMatrix);
        }

        ++point;
    }

}

void LineRendererComponent::CreatePoint()
{

    std::shared_ptr<RenderPoint> newPoint = std::make_shared<RenderPoint>();
    m_points.push_back(newPoint);

    newPoint->position = Vector3::Zero;
    newPoint->rotation = Quaternion::Identity;
    newPoint->width = 0.0f;
    newPoint->transformParent = nullptr;
}

std::unique_ptr<Component> LineRendererComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<LineRendererComponent> cloned = std::make_unique<LineRendererComponent>(m_uuid, newOwner);

    cloned->m_color = m_color;

    //TODO: Copy list of points

    return cloned;
}

void LineRendererComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    //TODO: Serialize Points and its values.


    if (archive.mode() == ArchiveMode::Output)
    {
        const auto& marks = m_color.getMarks();
        uint32_t markCount = static_cast<uint32_t>(marks.size());
        archive.beginArray(markCount, "ColorGradient");

        for (const ImGradientMark* mark : marks)
        {
            archive.beginObject();
            bool isAlphaMark = mark->alpha;
            float position = mark->position;
            archive.serialize(isAlphaMark, "IsAlphaMark");
            archive.serialize(position, "Position");

            if (mark->alpha)
            {
                float alpha = mark->color[0];
                archive.serialize(alpha, "Alpha");
            }
            else
            {
                uint32_t colorCount = 3;
                archive.beginArray(colorCount, "Color");
                float r = mark->color[0];
                float g = mark->color[1];
                float b = mark->color[2];
                archive.serialize(r, "");
                archive.serialize(g, "");
                archive.serialize(b, "");
                archive.endArray();
            }

            archive.endObject();
        }

        archive.endArray();

    }
    else
    {
        uint32_t markCount = 0;
        archive.beginArray(markCount, "ColorGradient");
        m_color.clearMarks();

        for (uint32_t i = 0; i < markCount; ++i)
        {
            archive.beginObject();

            bool isAlphaMark = false;
            float position = 0.f;
            archive.serialize(isAlphaMark, "IsAlphaMark");
            archive.serialize(position, "Position");

            if (isAlphaMark)
            {
                float alpha = 0.f;
                archive.serialize(alpha, "Alpha");
                m_color.addAlphaMark(position, alpha);
            }
            else
            {
                uint32_t colorCount = 3;
                archive.beginArray(colorCount, "Color");
                float r = 0.f, g = 0.f, b = 0.f;
                archive.serialize(r, "");
                archive.serialize(g, "");
                archive.serialize(b, "");
                archive.endArray();
                m_color.addMark(position, ImColor(r, g, b));
            }

            archive.endObject();
        }

        archive.endArray();

    }
}

void LineRendererComponent::debugDraw()
{
    if (!isActive() || !m_owner->GetActive()) //|| !m_textureAsset.isValid()
    {
        return;
    }
}
