#include "Globals.h"
#include "TrailComponent.h"
#include "GameObject.h"
#include "Transform.h"

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

    float color[4] = { m_startColor.x, m_startColor.y, m_startColor.z, m_startColor.w };
    if (ImGui::ColorEdit4("Starting color", color))
    {
        Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
        m_startColor = newColor;
       
    }

    color[0] = m_endColor.x; color[1] = m_endColor.y; color[2] = m_endColor.z; color[3] = m_endColor.w;
    if (ImGui::ColorEdit4("End color", color))
    {
        Vector4 newColor = Vector4(color[0], color[1], color[2], color[3]);
        m_endColor = newColor;
      
    }

}
