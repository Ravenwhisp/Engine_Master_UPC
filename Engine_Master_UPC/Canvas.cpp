#include "Globals.h"
#include "Canvas.h"
#include "JsonArchive.h"

#include <imgui.h>

Canvas::Canvas(UID id, GameObject* owner)
    : Component(id, ComponentType::CANVAS, owner)
{
}

void Canvas::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serializeStringEnum(renderMode, "RenderMode", CanvasRenderModeToString, StringToCanvasRenderMode);
    archive.serialize(zTest, "ZTest");
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



