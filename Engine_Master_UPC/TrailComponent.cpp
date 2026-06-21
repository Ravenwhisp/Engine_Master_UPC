#include "Globals.h"
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

void TrailComponent::update()
{

    if(m_points.size() <= 0)
    {
        CreatePoint();
    }

    Vector3 begining = m_points.front()->position;
    Vector3 end = m_owner->GetTransform()->getPosition();

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
        ++trailPoint;
    }

}

void TrailComponent::CreatePoint()
{
    std::shared_ptr<TrailPoint> newPoint = std::make_shared<TrailPoint>();
    m_points.push_back(newPoint);

    newPoint->position = m_owner->GetTransform()->getPosition();
    newPoint->rotation = m_owner->GetTransform()->getRotation();
    newPoint->lifeTime = m_pointLifetime;
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

rapidjson::Value TrailComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::PARTICLE_SYSTEM), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    //componentInfo.AddMember("TextureAssetId", m_textureAsset.getJson(domTree.GetAllocator()), domTree.GetAllocator());

    return componentInfo;
}

bool TrailComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    /*if (componentInfo.HasMember("TextureAssetId"))
    {
        m_textureAsset.deserializeJson(componentInfo["TextureAssetId"]);
    }*/

    return true;
}

void TrailComponent::debugDraw()
{
    if (!isActive() || !m_owner->GetActive() ) //|| !m_textureAsset.isValid()
    {
        return;
    }
}
