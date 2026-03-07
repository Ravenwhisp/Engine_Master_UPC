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

    clonedCanvas->isScreenSpace = this->isScreenSpace;
    clonedCanvas->setActive(this->isActive());

	return clonedCanvas;
}

void Canvas::drawUi()
{
    ImGui::Text("Canvas");
    ImGui::Checkbox("Screen Space", &isScreenSpace);
}

rapidjson::Value Canvas::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::CANVAS), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("ScreenSpace", isScreenSpace, domTree.GetAllocator());

    return componentInfo;
}

bool Canvas::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("ScreenSpace"))
    {
        isScreenSpace = componentInfo["ScreenSpace"].GetBool();
    }

    return true;
}

