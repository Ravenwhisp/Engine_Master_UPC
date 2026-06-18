#include "Globals.h"
#include "TrailComponent.h"
#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "ModuleInput.h"
#include "imgui_bezier.h"
#include "imgui_color_gradient.h"



TrailComponent::TrailComponent(UID id, GameObject* owner) : Component(id, ComponentType::TRAIL, owner)
{

    TrailPoint* firstPoint;

    m_points.push_back(firstPoint);

    firstPoint->position = m_owner->GetTransform()->getPosition();
    firstPoint->rotation = m_owner->GetTransform()->getRotation();
}

void TrailComponent::drawUi()
{
    ImGui::Text("Trail");

    // TRAIL INTERFACE

    // trail controller //


    ImGui::DragFloat("Start Width", &m_startWidth, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("End Width", &m_endWidth, 0.01f, 0.0f, 10.0f);

    ImGui::DragFloat("Min Vertex Distance", &m_spawnDistance, 0.1f, 0.05f, 5.0f);
    ImGui::DragFloat("Vertex Lifetime", &m_pointLifetime, 0.1f, 0.05f, 5.0f);

    // Color gradient editor
    if (ImGui::GradientButton(&m_colorOverTime)) {

        ImGui::OpenPopup("GradientEditorPopup");
    }

    ImGui::SameLine(); ImGui::Text("Color Gradient");


    if (ImGui::BeginPopup("GradientEditorPopup")) {

        //ImGui::PushID("GradientID"); // to avoid conflicting index error
        //size_t beforeMarkers = m_colorsOverTime.getMarks().size();

        ImGui::GradientEditor(&m_colorOverTime, m_draggingMark, m_selectedMark);

        if (app->getModuleInput()->isKeyJustPressed(Keyboard::Keys::Delete) && m_selectedMark != nullptr)
        {
            m_colorOverTime.removeMark(m_selectedMark);
        }

        //ImGui::PopID(); // (same, corresponding)

        ImGui::EndPopup();
    }

}
