#include "Globals.h"
#include "SceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>

#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "CameraModule.h"

#include "RenderModule.h"
#include "SceneModule.h"
#include "EditorToolbar.h"
#include "PlayToolbar.h"
#include "NavigationModule.h"

#include "Settings.h"

#include "GameObject.h"
#include "DebugDrawPass.h"
#include "LightDebugDraw.h"
#include "LightComponent.h"
#include "TriggerArea.h"
#include "Quadtree.h"

#include "CameraComponent.h"


SceneEditor::SceneEditor()
{
    m_cameraModule = app->getCameraModule();
    m_inputModule = app->getInputModule();

    m_settings = app->getSettings();

    m_editorToolbar = new EditorToolbar();
	m_playToolbar = new PlayToolbar();

    auto d3d12Module = app->getD3D12Module();
}

SceneEditor::~SceneEditor()
{
    delete m_editorToolbar;
	delete m_playToolbar;
}

void SceneEditor::update()
{

}

void SceneEditor::render()
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

        ImTextureID textureID = (ImTextureID)app->getRenderModule()->getGPUEditorScreenRT().ptr;
        ImGui::Image(textureID, m_size);
        
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    ImGuizmo::Enable(true);

    GameObject* selectedGameObject = app->getEditorModule()->getSelectedGameObject();
    if (selectedGameObject && m_cameraModule)
    {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        bool shouldShowGizmo = m_settings->sceneEditor.showGuizmo;

        EditorModule::SCENE_TOOL currentMode = app->getEditorModule()->getCurrentSceneTool();
        switch (currentMode) 
        {
            case EditorModule::SCENE_TOOL::MOVE:          op = ImGuizmo::TRANSLATE; break;
            case EditorModule::SCENE_TOOL::ROTATE:        op = ImGuizmo::ROTATE; break;
            case EditorModule::SCENE_TOOL::SCALE:         op = ImGuizmo::SCALE; break;
            case EditorModule::SCENE_TOOL::TRANSFORM:     op = ImGuizmo::UNIVERSAL; break;
            default: shouldShowGizmo = false; break;
        }

        if (shouldShowGizmo) 
        {
            Transform* transform = selectedGameObject->GetTransform();
            Matrix worldMatrix = transform->getGlobalMatrix();

            ImGuizmo::MODE gizmoMode = app->getEditorModule()->isGizmoLocal() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

            ImGuizmo::Manipulate(
                (float*)&m_cameraModule->getView(),
                (float*)&m_cameraModule->getProjection(),
                op,
                gizmoMode,
                (float*)&worldMatrix
            );

            if (ImGuizmo::IsUsing())
            {
                transform->setFromGlobalMatrix(worldMatrix);
            }
        }
    }

    m_isViewportHovered = ImGui::IsWindowHovered();
    m_isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

bool SceneEditor::resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_size.x) > 1.0f ||
        abs(contentRegion.y - m_size.y) > 1.0f) 
    {
        setSize(contentRegion);
        return true;
    }
    return false;
}

void SceneEditor::renderDebugDrawPass(ID3D12GraphicsCommandList* commandList)
{
    if (m_settings->sceneEditor.showGrid)
    {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    if (m_settings->sceneEditor.showAxis)
    {
        dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);
    }

    if (m_settings->sceneEditor.showQuadTree)
    {
        renderQuadtree();
    }

    for (GameObject* go : app->getSceneModule()->getAllGameObjects())
    {
        if (!go || !go->GetActive()) 
        {
            continue;
        }

        auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT);

        if (!light)
        {
            continue;
        }

        if (!light->isDebugDrawEnabled())
        {
            continue;
        }

        if (light->isDebugDrawDepthEnabled())
        {
            LightDebugDraw::drawLightWithDepth(*go);
        }
        else
        {
            LightDebugDraw::drawLightWithoutDepth(*go);
        }

        auto* area = go->GetComponentAs<TriggerArea>(ComponentType::CHANGE_SCENE_ON_TRIGGER);
        
        if (area) 
        {
            area->printArea();
        }
    }

    NavigationModule* nav = app->getNavigationModule();
    if (nav && nav->getDrawNavMesh() && nav->getNavMesh())
    {
        const auto& lines = nav->getNavMeshDebugLines();
        for (const auto& l : lines)
        {
            const Vector3 a = l.a + Vector3(0.0f, 0.03f, 0.0f);
            const Vector3 b = l.b + Vector3(0.0f, 0.03f, 0.0f);
            dd::line(ddConvert(a), ddConvert(b), dd::colors::Green);
        }
    }

    if (m_settings->sceneEditor.showModelBoundingBoxes)
    {
        for (const auto& renderer : app->getSceneModule()->getAllMeshRenderers())
        {
            drawBoundingBox(renderer->getBoundingBox(), dd::colors::Yellow);
        }
    }

    Matrix viewMatrix;
    Matrix projectionMatrix;

    if (app->getCurrentCameraPerspective())
    {
        viewMatrix = app->getCurrentCameraPerspective()->getViewMatrix();
        projectionMatrix = app->getCurrentCameraPerspective()->getProjectionMatrix();
    }
    else
    {
        viewMatrix = app->getCameraModule()->getView();
        projectionMatrix = app->getCameraModule()->getProjection();
    }
}

void SceneEditor::renderQuadtree()
{
    Quadtree* quadtree = app->getSceneModule()->getQuadtree();
    if (!quadtree)
    {
        return;
    }

    std::vector<BoundingRect> quadrants = quadtree->getQuadrants();
    for (const auto& rect : quadrants)
    {
        Vector3 extents(rect.width * 0.5f, 0.0f, rect.height * 0.5f);
		Vector3 center(rect.x + rect.width * 0.5f, 0.1f, rect.y + rect.height * 0.5f);

        
        float color[3];
        if (rect.m_debugIsCulled) 
        {
        dd::box(ddConvert(center), dd::colors::Red, extents.x * 2.0f, extents.y * 2.0f, extents.z * 2.0f);
	}
        else 
        {
            dd::box(ddConvert(center), dd::colors::Green, extents.x * 2.0f, extents.y * 2.0f, extents.z * 2.0f);
        }

        int minY = -10000;
        int maxY = 10000;

        center.y = (minY + maxY) * 0.5f;

        extents.y = (maxY - minY) * 0.5f;

        Vector3 min = center - extents;
        Vector3 max = center + extents;

        Vector3 bbPoints[8] =
        {
            Vector3(center.x - extents.x, center.y - extents.y, center.z - extents.z),
            Vector3(center.x - extents.x, center.y - extents.y, center.z + extents.z),
            Vector3(center.x - extents.x, center.y + extents.y, center.z - extents.z),
            Vector3(center.x - extents.x, center.y + extents.y, center.z + extents.z),
            Vector3(center.x + extents.x, center.y - extents.y, center.z - extents.z),
            Vector3(center.x + extents.x, center.y - extents.y, center.z + extents.z),
            Vector3(center.x + extents.x, center.y + extents.y, center.z - extents.z),
            Vector3(center.x + extents.x, center.y + extents.y, center.z + extents.z)
        };

        Engine::BoundingBox bb = Engine::BoundingBox(min, max, bbPoints);
        bb.render();
	}
}

void SceneEditor::drawBoundingBox(const Engine::BoundingBox& bbox, const ddVec3& color)
{
    const Vector3* c = bbox.getPoints();

    ddVec3 pts[8];

    for (int i = 0; i < 8; ++i)
    {
        pts[i][0] = c[i].x;
        pts[i][1] = c[i].y;
        pts[i][2] = c[i].z;
    }

    dd::box(pts, color, 0, false);
}