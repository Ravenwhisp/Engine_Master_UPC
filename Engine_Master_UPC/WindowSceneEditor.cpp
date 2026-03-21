#include "Globals.h"
#include "WindowSceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleCamera.h"

#include "ModuleRender.h"
#include "ModuleScene.h"
#include "EditorToolbar.h"
#include "PlayToolbar.h"

#include "Settings.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

#include "TriggerArea.h"

#include <WindowLogger.h>

WindowSceneEditor::WindowSceneEditor()
{
    m_moduleCamera = app->getModuleCamera();
    m_moduleInput = app->getModuleInput();

    m_settings = app->getSettings();

    m_editorToolbar = new EditorToolbar();
	m_playToolbar = new PlayToolbar();

    auto d3d12Module = app->getModuleD3D12();
}

WindowSceneEditor::~WindowSceneEditor()
{
    delete m_editorToolbar;
	delete m_playToolbar;
}

void WindowSceneEditor::update()
{

}

void WindowSceneEditor::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    float toolbarWidth = ImGui::GetContentRegionAvail().x;
	m_playToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();
    m_editorToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();
    ImGui::Separator();

    ImVec2 windowPos = ImGui::GetWindowPos();
    m_windowX = windowPos.x;
    m_windowY = windowPos.y;

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    if (contentRegion.x > 0 && contentRegion.y > 0) 
    {
        resize(contentRegion);

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        m_viewportX = imageTopLeft.x;
        m_viewportY = imageTopLeft.y;

        ImTextureID textureID = (ImTextureID)app->getModuleRender()->getGPUEditorScreenRT().ptr;
        ImGui::Image(textureID, m_size);
        
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    m_viewportPos = contentPos;

    ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    ImGuizmo::Enable(true);

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();

    if (selectedGameObject && m_moduleCamera)
    {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        bool shouldShowGizmo = m_settings->sceneEditor.showGuizmo;

        ModuleEditor::SCENE_TOOL currentMode = app->getModuleEditor()->getCurrentSceneTool();

        switch (currentMode) 
        {
        case ModuleEditor::SCENE_TOOL::MOVE:          op = ImGuizmo::TRANSLATE; break;
        case ModuleEditor::SCENE_TOOL::ROTATE:        op = ImGuizmo::ROTATE; break;
        case ModuleEditor::SCENE_TOOL::SCALE:         op = ImGuizmo::SCALE; break;
        case ModuleEditor::SCENE_TOOL::TRANSFORM:     op = ImGuizmo::UNIVERSAL; break;
        default: shouldShowGizmo = false; break;
        }

        if (shouldShowGizmo) 
        {
            Transform* transform = selectedGameObject->GetTransform();
            Matrix worldMatrix = transform->getGlobalMatrix();

            ImGuizmo::MODE gizmoMode = app->getModuleEditor()->isGizmoLocal() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

            ImGuizmo::Manipulate(
                (float*)&m_moduleCamera->getView(),
                (float*)&m_moduleCamera->getProjection(),
                op,
                gizmoMode,
                (float*)&worldMatrix
            );

            if (ImGuizmo::IsUsing())
            {
                transform->setFromGlobalMatrix(worldMatrix);
                PrefabUI::markTransformOverride(selectedGameObject);
            }
        }
    }

    m_isViewportHovered = ImGui::IsWindowHovered();
    m_isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

bool WindowSceneEditor::resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_size.x) > 1.0f ||
        abs(contentRegion.y - m_size.y) > 1.0f) 
    {
        setSize(contentRegion);
        return true;
    }
    return false;
}

