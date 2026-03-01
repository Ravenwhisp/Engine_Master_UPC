#include "Globals.h"
#include "UIText.h"
#include <imgui.h>

UIText::UIText(UID id, GameObject* owner)
    : Component(id, ComponentType::UITEXT, owner)
{
}

void UIText::drawUi()
{
    ImGui::Text("UIText");

    char buf[512];
    std::memset(buf, 0, sizeof(buf));

    if (!m_text.empty())
    {
        std::strncpy(buf, m_text.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
    }

    if (ImGui::InputText("Text", buf, IM_ARRAYSIZE(buf)))
    {
        m_text = buf;
    }

    ImGui::DragFloat("Scale", &m_scale, 0.01f, 0.1f, 5.0f);

    float color[4] = { m_color.x, m_color.y, m_color.z, m_color.w };
    if (ImGui::ColorEdit4("Color", color))
    {
        m_color = { color[0], color[1], color[2], color[3] };
    }
}

rapidjson::Value UIText::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::UITEXT), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    {
        rapidjson::Value textString(m_text.c_str(), domTree.GetAllocator());
        componentInfo.AddMember("Text", textString, domTree.GetAllocator());
    }

    componentInfo.AddMember("Scale", m_scale, domTree.GetAllocator());

    {
        rapidjson::Value colorData(rapidjson::kArrayType);
        colorData.PushBack(m_color.x, domTree.GetAllocator());
        colorData.PushBack(m_color.y, domTree.GetAllocator());
        colorData.PushBack(m_color.z, domTree.GetAllocator());
        colorData.PushBack(m_color.w, domTree.GetAllocator());
        componentInfo.AddMember("Color", colorData, domTree.GetAllocator());
    }

    return componentInfo;
}

bool UIText::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("Text"))
    {
        m_text = componentInfo["Text"].GetString();
    }

    if (componentInfo.HasMember("Scale"))
    {
        m_scale = componentInfo["Scale"].GetFloat();
    }

    if (componentInfo.HasMember("Color"))
    {
        m_color.x = componentInfo["Color"][0].GetFloat();
        m_color.y = componentInfo["Color"][1].GetFloat();
        m_color.z = componentInfo["Color"][2].GetFloat();
        m_color.w = componentInfo["Color"][3].GetFloat();
    }

    return true;
}