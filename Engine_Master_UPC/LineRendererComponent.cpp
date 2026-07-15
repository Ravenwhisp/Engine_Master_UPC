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


LineRendererComponent::LineRendererComponent(UID id, GameObject* owner) : Component(id, ComponentType::LINE_RENDERER, owner)
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
    
    //ImGui::Begin("Render Points");

    ImGui::BeginChild("List", ImVec2(220, 0), true);
   
    // Add button
    if (ImGui::Button("Add"))
    {
        CreatePoint();
        m_selectedPoint = (int)m_points.size() - 1;
    }

    ImGui::SameLine();

    // Remove button
    if (ImGui::Button("Remove"))
    {
        if (m_selectedPoint >= 0 &&
            m_selectedPoint < (int)m_points.size())
        {
            m_points.erase(m_points.begin() + m_selectedPoint);

            if (m_points.empty())
                m_selectedPoint = -1;
            else if (m_selectedPoint >= (int)m_points.size())
                m_selectedPoint = (int)m_points.size() - 1;
        }
    }

    ImGui::Separator();

    if (ImGui::BeginListBox("Points"))
    {
        for (int i = 0; i < (int)m_points.size(); i++)
        {
            bool selected = (m_selectedPoint == i);

            std::string label = "Point " + std::to_string(i);

            if (ImGui::Selectable(label.c_str(), selected))
                m_selectedPoint = i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndListBox();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Inspector", ImVec2(0, 0), true);

    if (m_selectedPoint >= 0 && m_selectedPoint < (int)m_points.size())
    {
        auto& point = *m_points[m_selectedPoint];
        bool disableTransform = (point.transformParent != nullptr);

        ImGui::SeparatorText("Transform");

        const char* ParentTransformName = point.transformParent ? point.transformParent->getOwner()->GetName().c_str() : "Drop Transform Here";

        ImGui::BeginGroup();

        ImGui::Text("Parent Transform");

        ImGui::Button(ParentTransformName, ImVec2(200, 0));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
            {
                Component* comp = *(Component**)payload->Data;

                if (comp && comp->getType() == ComponentType::TRANSFORM)
                {
                    point.transformParent = comp->getTransform();
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (point.transformParent)
        {
            ImGui::SameLine();

            if (ImGui::SmallButton("X"))
            {
                point.transformParent = nullptr;
            }
        }

        ImGui::EndGroup();

        ImGui::BeginDisabled(disableTransform);

        ImGui::DragFloat3("Position", &point.position.x, 0.1f);

        if (disableTransform && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Inherited from parent transform.");
        }

        if (ImGui::DragFloat3("Rotation", &point.editorEuler.x, 1.0f))
        {
            point.editorEuler.x = WrapAngle(point.editorEuler.x);
            point.editorEuler.y = WrapAngle(point.editorEuler.y);
            point.editorEuler.z = WrapAngle(point.editorEuler.z);

            point.rotation = Quaternion::CreateFromYawPitchRoll(
                XMConvertToRadians(point.editorEuler.y),
                XMConvertToRadians(point.editorEuler.x),
                XMConvertToRadians(point.editorEuler.z)
            );

            point.rotation.Normalize();
        }

        if (disableTransform && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Inherited from parent transform.");
        }

        ImGui::EndDisabled();

        ImGui::SeparatorText("Shape");

        ImGui::DragFloat( "Width", &point.width, 0.01f, 0.0f);
        
    }

    ImGui::EndChild();

    //ImGui::End();
    

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

float LineRendererComponent::WrapAngle(float angle)
{
    angle = std::fmod(angle, 360.0f);
    if (angle < 0.0f)
        angle += 360.0f;
    return angle;
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
