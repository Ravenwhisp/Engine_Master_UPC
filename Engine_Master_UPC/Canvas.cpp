#include "Globals.h"
#include "Canvas.h"

#include <imgui.h>

Canvas::Canvas(UID id, GameObject* owner)
    : Component(id, ComponentType::CANVAS, owner)
{
}

std::unique_ptr<Component> Canvas::clone(GameObject* newOwner) const
{
    std::unique_ptr<Canvas> clonedCanvas = std::make_unique<Canvas>(m_uuid, newOwner);

    clonedCanvas->renderMode = renderMode;
    clonedCanvas->zTest = zTest;
    clonedCanvas->setActive(this->isActive());

	return clonedCanvas;
}

void Canvas::drawUi()
{
    ImGui::Text("Canvas");
    const char* modes[] = { "Screen Space", "World Space", "World Space (Facing Camera)" };
    int current = static_cast<int>(renderMode);
    if (ImGui::Combo("Render Mode", &current, modes, IM_ARRAYSIZE(modes)))
    {
        renderMode = static_cast<CanvasRenderMode>(current);
    }
    
    if (renderMode != CanvasRenderMode::SCREEN_SPACE)
    {
        ImGui::Checkbox("Depth Test (Z-Test)", &zTest);
    }
}

rapidjson::Value Canvas::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::CANVAS), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("RenderMode", static_cast<int>(renderMode), domTree.GetAllocator());
    componentInfo.AddMember("ZTest", zTest, domTree.GetAllocator());

    return componentInfo;
}

bool Canvas::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("RenderMode"))
    {
        renderMode = static_cast<CanvasRenderMode>(componentInfo["RenderMode"].GetInt());
    }
    else if (componentInfo.HasMember("ScreenSpace"))
    {
        renderMode = componentInfo["ScreenSpace"].GetBool()
            ? CanvasRenderMode::SCREEN_SPACE
            : CanvasRenderMode::WORLD_SPACE;
    }

    if (componentInfo.HasMember("ZTest"))
    {
        zTest = componentInfo["ZTest"].GetBool();
    }

    return true;
}

