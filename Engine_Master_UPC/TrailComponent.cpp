#include "Globals.h"
#include "JsonArchive.h"
#include "TrailComponent.h"
#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "ModuleInput.h"
#include "imgui_bezier.h"
#include "imgui_color_gradient.h"
#include "ModuleTime.h"




TrailComponent::TrailComponent(UID id, GameObject* owner) : Component(id, ComponentType::TRAIL, owner)
{
    CreatePoint();

    m_colorOverTime.getMarks().clear(); // because it has default values that we don't want

    m_colorOverTime.addMark(0.f, ImColor(1.f, 1.f, 1.f, 1.f));
    m_colorOverTime.addAlphaMark(0.f, true);
    m_colorOverTime.addMark(1.f, ImColor(1.f, 1.f, 1.f, 1.f));
    m_colorOverTime.addAlphaMark(1.f, true);
    m_colorOverTime.setEditAlpha(true);
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

    drawBezierCurveUI(m_colorCurve);
}

void TrailComponent::update()
{

    if(m_points.size() <= 0)
    {
        CreatePoint();
    }

    Vector3 begining = m_points.front()->position;

    const Matrix& globalMatrix = m_owner->GetTransform()->getGlobalMatrix();
    Vector3 end = globalMatrix.Translation();

    float dist_x = begining.x - end.x;
    float dist_y = begining.y - end.y;
    float dist_z = begining.z - end.z;

    float distance_sqr = dist_x * dist_x + dist_y * dist_y + dist_z * dist_z;
    
    if (distance_sqr >= (m_spawnDistance * m_spawnDistance)) 
    {
        CreatePoint();
    }

    for (auto trailPoint = m_points.begin(); trailPoint != m_points.end(); )
    {
        trailPoint->get()->lifeTime -= app->getModuleTime()->deltaTime();

        if (trailPoint->get()->lifeTime <= 0.0f)
        {
            trailPoint = m_points.erase(trailPoint);
            continue;
        }  

        trailPoint->get()->width = std::lerp(m_endWidth, m_startWidth, trailPoint->get()->lifeTime / m_pointLifetime);

        float colorScale = 1.f - trailPoint->get()->lifeTime / m_pointLifetime; // to start with 0
        float bezierScale = ImGui::BezierValue(colorScale, m_colorCurve);

        m_colorOverTime.getColorAt(bezierScale, &trailPoint->get()->color.x);

        ++trailPoint;
    }

}

void TrailComponent::CreatePoint()
{
    std::shared_ptr<TrailPoint> newPoint = std::make_shared<TrailPoint>();
    m_points.push_back(newPoint);

    const Matrix& globalMatrix = m_owner->GetTransform()->getGlobalMatrix();
    newPoint->position = globalMatrix.Translation();
    newPoint->rotation = Quaternion::CreateFromRotationMatrix(globalMatrix);
    newPoint->lifeTime = m_pointLifetime;

    m_colorOverTime.getColorAt(0.f, &newPoint->color.x);
}

std::unique_ptr<Component> TrailComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<TrailComponent> cloned = std::make_unique<TrailComponent>(m_uuid, newOwner);

   cloned->m_startWidth    = m_startWidth;
   cloned->m_endWidth      = m_endWidth;
   cloned->m_spawnDistance = m_spawnDistance;
   cloned->m_pointLifetime = m_pointLifetime;
   cloned->m_colorOverTime = m_colorOverTime;

    return cloned;
}

void TrailComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serialize(m_startWidth, "StartWidth");
    archive.serialize(m_endWidth, "EndWidth");
    archive.serialize(m_spawnDistance, "SpawnDistance");
    archive.serialize(m_pointLifetime, "PointLifetime");

    if (archive.mode() == ArchiveMode::Output)
    {
        const auto& marks = m_colorOverTime.getMarks();
        uint32_t markCount = static_cast<uint32_t>(marks.size());
        archive.beginArray(markCount, "ColorOverTime");

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
        archive.beginArray(markCount, "ColorOverTime");
        m_colorOverTime.clearMarks();

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
                m_colorOverTime.addAlphaMark(position, alpha);
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
                m_colorOverTime.addMark(position, ImColor(r, g, b));
            }

            archive.endObject();
        }

        archive.endArray();
    }
}

void TrailComponent::debugDraw()
{
    if (!isActive() || !m_owner->GetActive() ) //|| !m_textureAsset.isValid()
    {
        return;
    }
}

bool TrailComponent::drawBezierCurveUI(float* curveData)
{
    bool parameterChanged = false;

    // (Values between 0 and 1)
    if (ImGui::Bezier("Curve##Color", curveData))
    {
        parameterChanged = true;
    }

    // We add some buttons to quickly change to predefined setups
    if (ImGui::Button("Linear##Color"))
    {
        curveData[0] = 0.000f; curveData[1] = 0.000f; curveData[2] = 1.000f; curveData[3] = 1.000f;
        parameterChanged = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("EaseIn##Color"))
    {
        curveData[0] = 0.470f; curveData[1] = 0.000f; curveData[2] = 0.745f; curveData[3] = 0.715f;
        parameterChanged = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("EaseOut##Color"))
    {
        curveData[0] = 0.390f; curveData[1] = 0.575f; curveData[2] = 0.565f; curveData[3] = 1.000f;
        parameterChanged = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("EaseInOut##Color"))
    {
        curveData[0] = 0.445f; curveData[1] = 0.050f; curveData[2] = 0.550f; curveData[3] = 0.950f;
        parameterChanged = true;
    }

    return parameterChanged;
}
