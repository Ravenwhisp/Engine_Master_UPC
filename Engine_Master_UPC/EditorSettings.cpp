#include "Globals.h"
#include "EditorSettings.h"
#include "Application.h"
#include "Settings.h"
#include "RenderModule.h"


EditorSettings::EditorSettings()
{
    m_settings = app->getSettings();
}

void EditorSettings::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr()))
    {
        ImGui::End();
        return;
    }

    ImGui::Separator();
    drawCameraSettings();
    ImGui::Separator();
    drawSceneSettings();

    ImGui::End();
}

void EditorSettings::drawCameraSettings() {
    if (ImGui::CollapsingHeader("Camera")) {
        if (ImGui::CollapsingHeader("Pan")) {
            ImGui::DragFloat("Pan Speed", &m_settings->camera.panSpeed, 0.0001f, 0.00001f, 0.05f);
            ImGui::Checkbox("Invert X###PanInvX", &m_settings->camera.panInvertX);
            ImGui::Checkbox("Invert Y###PanInvY", &m_settings->camera.panInvertY);
        }

        if (ImGui::CollapsingHeader("Orbit")) {
            ImGui::DragFloat("Orbit Speed", &m_settings->camera.orbitSpeed, 0.0001f, 0.0001f, 0.05f);
            ImGui::Checkbox("Invert X###OrbInvX", &m_settings->camera.orbitInvertX);
            ImGui::Checkbox("Invert Y###OrbInvY", &m_settings->camera.orbitInvertY);

            ImGui::DragFloat("Min Distance", &m_settings->camera.minDistance, 0.1f, 0.01f, 1000.0f);
            ImGui::DragFloat("Max Distance", &m_settings->camera.maxDistance, 0.1f, 0.1f, 5000.0f);
        }

        if (ImGui::CollapsingHeader("Zoom")) {
            ImGui::DragFloat("Zoom Drag Speed", &m_settings->camera.zoomDragSpeed, 0.0001f, 0.00001f, 0.1f);
            ImGui::DragFloat("Zoom Wheel Speed", &m_settings->camera.zoomWheelSpeed, 0.0001f, 0.000001f, 0.1f);
        }

        if (ImGui::CollapsingHeader("Flythrough")) {
            ImGui::DragFloat("Move Speed", &m_settings->camera.flyMoveSpeed, 0.1f, 0.1f, 500.0f);
            ImGui::DragFloat("Boost Mult", &m_settings->camera.flyBoostMult, 0.1f, 1.0f, 20.0f);
            ImGui::DragFloat("Rotation Speed", &m_settings->camera.flyRotSpeed, 0.0001f, 0.0001f, 0.05f);

            ImGui::Checkbox("Invert X###FlyInvX", &m_settings->camera.flyInvertX);
            ImGui::Checkbox("Invert Y###FlyInvY", &m_settings->camera.flyInvertY);

            ImGui::DragFloat("Pitch Clamp", &m_settings->camera.flyPitchClamp, 0.01f, 0.1f, XM_PIDIV2 - 0.01f);
        }
    }
}

void EditorSettings::drawSceneSettings() {
    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Checkbox("Show Grid###SceneShowGrid", &m_settings->sceneEditor.showGrid);
        ImGui::Checkbox("Show Axis###SceneShowAxis", &m_settings->sceneEditor.showAxis);
        ImGui::Checkbox("Show Gizmo###SceneShowGizmo", &m_settings->sceneEditor.showGuizmo);
        ImGui::Checkbox("Show Quadtree###SceneShowQuadtree", &m_settings->sceneEditor.showQuadTree);
    }
}
