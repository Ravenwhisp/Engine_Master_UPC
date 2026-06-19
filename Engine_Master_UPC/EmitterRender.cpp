#include "Globals.h"
#include "EmitterRender.h"
#include <imgui.h>

bool EmitterRender::drawUi()
{
    bool parameterChanged = false;

    if (ImGui::CollapsingHeader("Render"))
    {
        const char* options[] = { "Billboard", "Horizontal", "Vertical" };
        int currentIndex = static_cast<int>(m_renderMode);
        if (ImGui::Combo("Orientation", &currentIndex, options, IM_ARRAYSIZE(options)))
        {
            m_renderMode = static_cast<RenderMode>(currentIndex);
            parameterChanged = true;
        }


        parameterChanged |= ImGui::DragInt("Render layer", &m_layer, 1.f);
    }

    return parameterChanged;
}

rapidjson::Value EmitterRender::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value moduleInfo(rapidjson::kObjectType);

    moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::RENDER), domTree.GetAllocator());

    moduleInfo.AddMember("RenderMode", unsigned int(m_renderMode), domTree.GetAllocator());

    moduleInfo.AddMember("RenderLayer", m_layer, domTree.GetAllocator());

    return moduleInfo;
}

bool EmitterRender::deserializeJSON(const rapidjson::Value& moduleInfo)
{
    if (moduleInfo.HasMember("RenderMode"))
    {
        m_renderMode = static_cast<RenderMode>(moduleInfo["RenderMode"].GetUint());
    }

    if (moduleInfo.HasMember("RenderLayer")) {
        m_layer = moduleInfo["RenderLayer"].GetInt();
    }

    return true;
}