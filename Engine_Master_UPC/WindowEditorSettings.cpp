#include "Globals.h"
#include "WindowEditorSettings.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleScripts.h"

#include "Settings.h"
#include "Scene.h"
#include "Quadtree.h"

WindowEditorSettings::WindowEditorSettings()
{
    m_settings = app->getSettings();
}

void WindowEditorSettings::drawInternal()
{
    ImGui::Separator();
    drawEngineInformation();
    ImGui::Separator();
    drawCameraSettings();
    ImGui::Separator();
    drawSceneSettings();
    ImGui::Separator();
    drawFrustumCullingSettings();
    ImGui::Separator();
    drawScriptsSettings();

    drawScriptReloadModal();
}

void WindowEditorSettings::drawEngineInformation()
{
    std::string str = "Engine versions " + m_settings->engine.version;
    ImGui::Text(str.c_str());
}

void WindowEditorSettings::drawCameraSettings()
{
    if (ImGui::CollapsingHeader("Camera"))
    {
        if (ImGui::CollapsingHeader("Pan"))
        {
            ImGui::DragFloat("Pan Speed", &m_settings->camera.panSpeed, 0.0001f, 0.00001f, 0.05f);
            ImGui::Checkbox("Invert X###PanInvX", &m_settings->camera.panInvertX);
            ImGui::Checkbox("Invert Y###PanInvY", &m_settings->camera.panInvertY);
        }

        if (ImGui::CollapsingHeader("Orbit"))
        {
            ImGui::DragFloat("Orbit Speed", &m_settings->camera.orbitSpeed, 0.0001f, 0.0001f, 0.05f);
            ImGui::Checkbox("Invert X###OrbInvX", &m_settings->camera.orbitInvertX);
            ImGui::Checkbox("Invert Y###OrbInvY", &m_settings->camera.orbitInvertY);
            ImGui::DragFloat("Min Distance", &m_settings->camera.minDistance, 0.1f, 0.01f, 1000.0f);
            ImGui::DragFloat("Max Distance", &m_settings->camera.maxDistance, 0.1f, 0.1f, 5000.0f);
        }

        if (ImGui::CollapsingHeader("Zoom"))
        {
            ImGui::DragFloat("Zoom Drag Speed", &m_settings->camera.zoomDragSpeed, 0.0001f, 0.00001f, 0.1f);
            ImGui::DragFloat("Zoom Wheel Speed", &m_settings->camera.zoomWheelSpeed, 0.0001f, 0.000001f, 0.1f);
        }

        if (ImGui::CollapsingHeader("Flythrough"))
        {
            ImGui::DragFloat("Move Speed", &m_settings->camera.flyMoveSpeed, 0.1f, 0.1f, 500.0f);
            ImGui::DragFloat("Boost Mult", &m_settings->camera.flyBoostMult, 0.1f, 1.0f, 20.0f);
            ImGui::DragFloat("Rotation Speed", &m_settings->camera.flyRotSpeed, 0.0001f, 0.0001f, 0.05f);
            ImGui::Checkbox("Invert X###FlyInvX", &m_settings->camera.flyInvertX);
            ImGui::Checkbox("Invert Y###FlyInvY", &m_settings->camera.flyInvertY);
            ImGui::DragFloat("Pitch Clamp", &m_settings->camera.flyPitchClamp, 0.01f, 0.1f, XM_PIDIV2 - 0.01f);
        }
    }
}

void WindowEditorSettings::drawSceneSettings()
{
    if (ImGui::CollapsingHeader("Scene"))
    {
        ImGui::Checkbox("Show Grid###SceneShowGrid", &m_settings->sceneEditor.showGrid);
        ImGui::Checkbox("Show Axis###SceneShowAxis", &m_settings->sceneEditor.showAxis);
        ImGui::Checkbox("Show Gizmo###SceneShowGizmo", &m_settings->sceneEditor.showGuizmo);

        ImGui::Checkbox("Show Quadtree###SceneShowQuadtree", &m_settings->sceneEditor.showQuadTree);

        ImGui::Checkbox("Show Model Bounding Boxes###ModelShowBoundingBoxes", &m_settings->sceneEditor.showModelBoundingBoxes);
        ImGui::Checkbox("Show NavPath###SceneShowNavPath", &m_settings->sceneEditor.showNavPath);
        ImGui::Checkbox("Show Light Component###SceneLightComponent", &m_settings->sceneEditor.showLightComponent);
        ImGui::Checkbox("Show Camera Frustum###SceneCameraFrustum", &m_settings->sceneEditor.showCameraFrustum);
    }
}

void WindowEditorSettings::drawFrustumCullingSettings()
{
    if (ImGui::CollapsingHeader("Frustum culling"))
    {
        ImGui::Checkbox("Debug enable###FrustumCullingEnabled", &m_settings->frustumCulling.debugFrustumCulling);
        ImGui::DragFloat("Quadtree extra X size", &m_settings->frustumCulling.quadtreeXExtraSize, 1.f, 0.f, 100.f);
        ImGui::DragFloat("Quadtree extra Z size", &m_settings->frustumCulling.quadtreeZExtraSize, 1.f, 0.f, 100.f);
    }

    if (m_settings->frustumCulling.debugFrustumCulling)
    {
        if (!app->getModuleScene()->getScene()->getDefaultCamera()) {
            m_settings->frustumCulling.debugFrustumCulling = false;
            DEBUG_WARN("Cannot debug frustum culling there is no default camera set in the scene.");
        }

        if (!app->getModuleScene()->getQuadtree()->getIsBuilded()) {
            m_settings->frustumCulling.debugFrustumCulling = false;
            DEBUG_WARN("Cannot debug frustum culling because quadtree is not builded.");
        }
    }
}

void WindowEditorSettings::drawScriptsSettings()
{
    if (!ImGui::CollapsingHeader("Scripts Reloading"))
    {
        return;
    }

    ModuleScripts* moduleScripts = app->getModuleScripts();

    ScriptBuildSettings& buildSettings = moduleScripts->getScriptBuildSettings();

    if (!m_scriptBuildSettingsSynced)
    {
        std::strncpy(m_scriptProjectPathBuffer.data(), buildSettings.projectPath.c_str(), m_scriptProjectPathBuffer.size());

        m_scriptProjectPathBuffer[m_scriptProjectPathBuffer.size() - 1] = '\0';

        std::strncpy(m_scriptSolutionDirBuffer.data(), buildSettings.solutionDir.c_str(), m_scriptSolutionDirBuffer.size());

        m_scriptSolutionDirBuffer[m_scriptSolutionDirBuffer.size() - 1] = '\0';

        m_scriptBuildSettingsSynced = true;
    }

    ImGui::TextUnformatted("Script Build Settings");

    ImGui::InputText("Project Path", m_scriptProjectPathBuffer.data(), m_scriptProjectPathBuffer.size());

    ImGui::InputText("Solution Dir", m_scriptSolutionDirBuffer.data(), m_scriptSolutionDirBuffer.size());

    if (ImGui::Button("Save Script Build Settings"))
    {
        buildSettings.projectPath = m_scriptProjectPathBuffer.data();
        buildSettings.solutionDir = m_scriptSolutionDirBuffer.data();

        moduleScripts->saveScriptBuildSettings();
    }

    ImGui::Separator();

    if (ImGui::Button("Build & Reload Game Scripts"))
    {
        buildSettings.projectPath = m_scriptProjectPathBuffer.data();
        buildSettings.solutionDir = m_scriptSolutionDirBuffer.data();

        moduleScripts->requestBuildAndReloadGameScriptsDll();
    }
}

void WindowEditorSettings::drawScriptReloadModal()
{
    ModuleScripts* moduleScripts = app->getModuleScripts();

    const ScriptReloadState reloadState = moduleScripts->getScriptReloadState();

    const bool shouldShowModal =
        reloadState == ScriptReloadState::Building ||
        reloadState == ScriptReloadState::Reloading ||
        reloadState == ScriptReloadState::BuildFailed ||
        reloadState == ScriptReloadState::ReloadFailed ||
        reloadState == ScriptReloadState::Completed;

    if (!shouldShowModal)
    {
        return;
    }

    static constexpr const char* MODAL_NAME = "GameScripts Reload";

    ImGui::OpenPopup(MODAL_NAME);

    if (ImGui::BeginPopupModal(MODAL_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
    {
        switch (reloadState)
        {
        case ScriptReloadState::Building:
            ImGui::TextUnformatted("Building GameScripts...");
            ImGui::Spacing();
            ImGui::TextDisabled("The editor is temporarily blocked while scripts are compiling.");
            break;

        case ScriptReloadState::Reloading:
            ImGui::TextUnformatted("Reloading GameScripts...");
            ImGui::Spacing();
            ImGui::TextDisabled("Restoring script instances and scene references.");
            break;

        case ScriptReloadState::Completed:
            ImGui::TextUnformatted("GameScripts reloaded successfully.");
            ImGui::Spacing();

            if (ImGui::Button("OK"))
            {
                moduleScripts->clearScriptReloadResult();
                ImGui::CloseCurrentPopup();
            }
            break;

        case ScriptReloadState::BuildFailed:
            ImGui::TextUnformatted("GameScripts build failed.");
            ImGui::Spacing();
            ImGui::TextDisabled("Current scripts remain loaded.");
            ImGui::TextDisabled("Check ScriptsBuild.log for the full build output.");

            if (ImGui::Button("OK"))
            {
                moduleScripts->clearScriptReloadResult();
                ImGui::CloseCurrentPopup();
            }
            break;

        case ScriptReloadState::ReloadFailed:
            ImGui::TextUnformatted("GameScripts reload failed.");
            ImGui::Spacing();
            ImGui::TextDisabled("Check the console for details.");

            if (ImGui::Button("OK"))
            {
                moduleScripts->clearScriptReloadResult();
                ImGui::CloseCurrentPopup();
            }
            break;

        case ScriptReloadState::Idle:
        default:
            break;
        }

        ImGui::EndPopup();
    }
}
