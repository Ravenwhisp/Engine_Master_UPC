#include "Globals.h"
#include "ViewGameDebug.h"

#include "imgui.h"

#include "Application.h"
#include "Settings.h"

ViewGameDebug::ViewGameDebug()
{
    m_settings = app->getSettings();
}

void ViewGameDebug::render()
{
    ImGui::Begin("Debug (F3 to close)");

    ImGui::Text("Engine version %s", m_settings->engine.version.c_str());
    
    ImGui::Separator();
    ImGui::Checkbox("Show Grid", &m_settings->sceneEditor.showGrid);
    ImGui::Checkbox("Show Axis", &m_settings->sceneEditor.showAxis);
    ImGui::Checkbox("Show QuadTree", &m_settings->sceneEditor.showQuadTree);
    ImGui::Checkbox("Show Model Bounding Boxes", &m_settings->sceneEditor.showModelBoundingBoxes);

    ImGui::Separator();
    ImGui::Checkbox("Frustum culling", &m_settings->frustumCulling.debugFrustumCulling);

    //ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Show FPS", &m_settings->debugGame.showFPS);
    ImGui::Checkbox("Show Frame time", &m_settings->debugGame.showFrametime);
    ImGui::Checkbox("Show triangles number", &m_settings->debugGame.showTrianglesNumber);

    ImGui::End();
}