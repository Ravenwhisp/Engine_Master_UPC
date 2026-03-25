#include "Globals.h"
#include "WindowSceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleCamera.h"
#include "ModuleResources.h"

#include "ModuleRender.h"
#include "ModuleScene.h"
#include "EditorToolbar.h"
#include "PlayToolbar.h"

#include "Settings.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

#include <WindowLogger.h>
#include <PrefabUI.h>
#include "RenderSurface.h"
#include "Texture.h"

WindowSceneEditor::WindowSceneEditor()
{
    m_moduleCamera = app->getModuleCamera();
    m_settings = app->getSettings();

    m_editorToolbar = new EditorToolbar();
    m_playToolbar = new PlayToolbar();

    auto d3d12Module = app->getModuleD3D12();

    m_surface.reset(app->getModuleResources()->createRenderSurface(m_size.x, m_size.y));
}

WindowSceneEditor::~WindowSceneEditor()
{
    delete m_editorToolbar;
    delete m_playToolbar;
}

void WindowSceneEditor::drawInternal()
{
    float toolbarWidth = ImGui::GetContentRegionAvail().x;

    m_playToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();

    m_editorToolbar->DrawCentered(toolbarWidth);
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::BeginChild("SceneViewport", ImGui::GetContentRegionAvail(), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    // Guard against zero-sized dock nodes.
    if (viewportSize.x < 1.0f || viewportSize.y < 1.0f)
    {
        ImGui::EndChild();
        ImGui::PopStyleVar();
        return;
    }

    resize(viewportSize);

    app->getModuleRender()->registerViewport( m_surface.get(), ModuleRender::ViewportType::EDITOR, viewportSize.x, viewportSize.y);

    // Capture the screen-space origin of the image before drawing it.
    ImVec2 imagePos = ImGui::GetCursorScreenPos();
    m_viewportX = imagePos.x;
    m_viewportY = imagePos.y;
    m_size = viewportSize;

    ImTextureID textureID = (ImTextureID)m_surface->getTexture(RenderSurface::COLOR_0)->getSRV().gpu.ptr;

    ImGui::Image(textureID, viewportSize);

    m_isViewportHovered = ImGui::IsItemHovered();
    m_isViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(m_viewportX, m_viewportY, viewportSize.x, viewportSize.y);
    ImGuizmo::Enable(true);

    drawGizmo();

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void WindowSceneEditor::drawGizmo()
{
    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
    if (!selected || !m_moduleCamera)
        return;

    if (!m_settings->sceneEditor.showGuizmo)
        return;

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;

    switch (app->getModuleEditor()->getCurrentSceneTool())
    {
    case ModuleEditor::MOVE:      operation = ImGuizmo::TRANSLATE; break;
    case ModuleEditor::ROTATE:    operation = ImGuizmo::ROTATE;    break;
    case ModuleEditor::SCALE:     operation = ImGuizmo::SCALE;     break;
    case ModuleEditor::TRANSFORM: operation = ImGuizmo::UNIVERSAL; break;
    default: return;
    }

    Transform* transform = selected->GetTransform();
    Matrix     worldMatrix = transform->getGlobalMatrix();

    ImGuizmo::MODE mode = app->getModuleEditor()->isGizmoLocal() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    ImGuizmo::Manipulate(
        (float*)&m_moduleCamera->getView(),
        (float*)&m_moduleCamera->getProjection(),
        operation, mode,
        (float*)&worldMatrix);

    if (ImGuizmo::IsUsing())
    {
        transform->setFromGlobalMatrix(worldMatrix);
        PrefabUI::markTransformOverride(selected);
    }
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

void WindowSceneEditor::debugDraw()
{
    const Settings* s = app->getSettings();

    if (s->sceneEditor.showGrid)
    {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    if (s->sceneEditor.showAxis)
    {
        dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);
    }
}